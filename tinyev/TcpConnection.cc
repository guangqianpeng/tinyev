//
// Created by frank on 17-9-1.
//

#include <cassert>
#include <unistd.h>

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>

using namespace ev;

namespace ev
{

enum ConnectionState
{
    kConnecting,
    kConnected,
    kDisconnecting,
    kDisconnected
};

void defaultThreadInitCallback(size_t index)
{
    TRACE("EventLoop thread #%lu started", index);
}

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    INFO("connection %s -> %s %s",
         conn->peer().toIpPort().c_str(),
         conn->local().toIpPort().c_str(),
         conn->connected() ? "up" : "down");
}

void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer& buffer)
{
    TRACE("connection %s -> %s recv %lu bytes",
          conn->peer().toIpPort().c_str(),
          conn->local().toIpPort().c_str(),
          buffer.readableBytes());
    buffer.retrieveAll();
}

}

TcpConnection::TcpConnection(EventLoop *loop, int sockfd,
                             const InetAddress& local,
                             const InetAddress& peer)
        : loop_(loop),
          sockfd_(sockfd),
          channel_(loop, sockfd_),
          state_(kConnecting),
          local_(local),
          peer_(peer),
          highWaterMark_(0)
{
    channel_.setReadCallback([this](){handleRead();});
    channel_.setWriteCallback([this](){handleWrite();});
    channel_.setCloseCallback([this](){handleClose();});
    channel_.setErrorCallback([this](){handleError();});

    TRACE("TcpConnection() %s fd=%d", name().c_str(), sockfd);
}

TcpConnection::~TcpConnection()
{
    assert(state_ == kDisconnected);
    ::close(sockfd_);

    TRACE("~TcpConnection() %s fd=%d", name().c_str(), sockfd_);
}

void TcpConnection::connectEstablished()
{
    assert(state_ == kConnecting);
    state_ = kConnected;
    channel_.tie(shared_from_this());
    channel_.enableRead();
}

bool TcpConnection::connected() const
{ return state_ == kConnected; }

bool TcpConnection::disconnected() const
{ return state_ == kDisconnected; }

void TcpConnection::send(std::string_view data)
{
    send(data.data(), data.length());
}

void TcpConnection::send(const char *data, size_t len)
{
    if (state_ != kConnected) {
        WARN("TcpConnection::send() not connected, give up send");
        return;
    }
    if (loop_->isInLoopThread()) {
        sendInLoop(data, len);
    }
    else {
        loop_->queueInLoop(
                [ptr = shared_from_this(), str = std::string(data, data+len)]()
                { ptr->sendInLoop(str);});
    }
}

void TcpConnection::sendInLoop(const char *data, size_t len)
{
    loop_->assertInLoopThread();
    // kDisconnecting is OK
    if (state_ == kDisconnected) {
        WARN("TcpConnection::sendInLoop() disconnected, give up send");
        return;
    }
    ssize_t n = 0;
    size_t remain = len;
    bool faultError = false;
    if (!channel_.isWriting()) {
        assert(outputBuffer_.readableBytes() == 0);
        n = ::write(sockfd_, data, len);
        if (n == -1) {
            if (errno != EAGAIN) {
                SYSERR("TcpConnection::write()");
                if (errno == EPIPE || errno == ECONNRESET)
                    faultError = true;
            }
            n = 0;
        }
        else {
            remain -= static_cast<size_t>(n);
            if (remain == 0 && writeCompleteCallback_) {
                // user may send data in writeCompleteCallback_
                // queueInLoop can break the chain
                loop_->queueInLoop(std::bind(
                        writeCompleteCallback_, shared_from_this()));
            }
        }
    }
    // todo: add highWaterMark
    if (!faultError && remain > 0) {
        if (highWaterMarkCallback_) {
            size_t oldLen = outputBuffer_.readableBytes();
            size_t newLen = oldLen + remain;
            if (oldLen < highWaterMark_ && newLen >= highWaterMark_)
                loop_->queueInLoop(std::bind(
                        highWaterMarkCallback_, shared_from_this(), newLen));
        }
        outputBuffer_.append(data + n, remain);
        channel_.enableWrite();
    }
}

void TcpConnection::sendInLoop(const std::string& message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::send(Buffer& buffer)
{
    if (state_ != kConnected) {
        WARN("TcpConnection::send() not connected, give up send");
        return;
    }
    if (loop_->isInLoopThread()) {
        sendInLoop(buffer.peek(), buffer.readableBytes());
        buffer.retrieveAll();
    }
    else {
        loop_->queueInLoop(
                [ptr = shared_from_this(), str = buffer.retrieveAllAsString()]()
                { ptr->sendInLoop(str); });
    }
}

void TcpConnection::shutdown()
{
    assert(state_ <= kDisconnecting);
    if (stateAtomicGetAndSet(kDisconnecting) == kConnected) {
        if (loop_->isInLoopThread())
            shutdownInLoop();
        else {
            loop_->queueInLoop(std::bind(
                    &TcpConnection::shutdownInLoop, shared_from_this()));
        }
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (state_ != kDisconnected && !channel_.isWriting()) {
        if (::shutdown(sockfd_, SHUT_WR) == -1)
            SYSERR("TcpConnection:shutdown()");
    }
}

void TcpConnection::forceClose()
{
    if (state_ != kDisconnected) {
        if (stateAtomicGetAndSet(kDisconnecting) != kDisconnected) {
            loop_->queueInLoop(std::bind(
                    &TcpConnection::forceCloseInLoop, shared_from_this()));
        }
    }
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ != kDisconnected) {
        handleClose();
    }
}

void TcpConnection::stopRead()
{
    loop_->runInLoop([this]() {
        if (channel_.isReading())
            channel_.disableRead();
    });
}

void TcpConnection::startRead()
{
    loop_->runInLoop([this]() {
        if (!channel_.isReading())
            channel_.enableRead();
    });
}

int TcpConnection::stateAtomicGetAndSet(int newState)
{
    return __atomic_exchange_n(&state_, newState, __ATOMIC_SEQ_CST);
}

void TcpConnection::handleRead()
{
    loop_->assertInLoopThread();
    assert(state_ != kDisconnected);
    int savedErrno;
    ssize_t n = inputBuffer_.readFd(sockfd_, &savedErrno);
    if (n == -1) {
        errno = savedErrno;
        SYSERR("TcpConnection::read()");
        handleError();
    }
    else if (n == 0)
        handleClose();
    else
        messageCallback_(shared_from_this(), inputBuffer_);
}

void TcpConnection::handleWrite()
{
    if (state_ == kDisconnected) {
        WARN("TcpConnection::handleWrite() disconnected, "
                     "give up writing %lu bytes", outputBuffer_.readableBytes());
        return;
    }
    assert(outputBuffer_.readableBytes() > 0);
    assert(channel_.isWriting());
    ssize_t n = ::write(sockfd_, outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n == -1) {
        SYSERR("TcpConnection::write()");
    }
    else {
        outputBuffer_.retrieve(static_cast<size_t>(n));
        if (outputBuffer_.readableBytes() == 0) {
            channel_.disableWrite();
            if (state_ == kDisconnecting)
                shutdownInLoop();
            if (writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(
                        writeCompleteCallback_, shared_from_this()));
            }
        }
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected ||
           state_ == kDisconnecting);
    state_ = kDisconnected;
    loop_->removeChannel(&channel_);
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    int err;
    socklen_t len = sizeof(err);
    int ret = getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &err, &len);
    if (ret != -1)
        errno = err;
    SYSERR("TcpConnection::handleError()");
}
