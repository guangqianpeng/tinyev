//
// Created by frank on 17-9-4.
//

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/TcpClient.h>

using namespace ev;

TcpClient::TcpClient(EventLoop* loop, const InetAddress& peer)
        : loop_(loop),
          connected_(false),
          peer_(peer),
          retryTimer_(nullptr),
          connector_(new Connector(loop, peer)),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback)
{
    connector_->setNewConnectionCallback(std::bind(
            &TcpClient::newConnection, this, _1, _2, _3));
}

TcpClient::~TcpClient()
{
    if (connection_ && !connection_->disconnected())
        connection_->forceClose();
    if (retryTimer_ != nullptr) {
        loop_->cancelTimer(retryTimer_);
    }
}

void TcpClient::start()
{
    loop_->assertInLoopThread();
    connector_->start();
    retryTimer_ = loop_->runEvery(3s, [this](){ retry(); });
}

void TcpClient::retry()
{
    loop_->assertInLoopThread();
    if (connected_) {
        return;
    }

    WARN("TcpClient::retry() reconnect %s...", peer_.toIpPort().c_str());
    connector_ = std::make_unique<Connector>(loop_, peer_);
    connector_->setNewConnectionCallback(std::bind(
            &TcpClient::newConnection, this, _1, _2, _3));
    connector_->start();
}

void TcpClient::newConnection(int connfd, const InetAddress& local, const InetAddress& peer)
{
    loop_->assertInLoopThread();
    loop_->cancelTimer(retryTimer_);
    retryTimer_ = nullptr; // avoid duplicate cancel
    connected_ = true;
    auto conn = std::make_shared<TcpConnection>
            (loop_, connfd, local, peer);
    connection_ = conn;
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallBack(std::bind(
            &TcpClient::closeConnection, this, _1));
    // enable and tie channel
    conn->connectEstablished();
    connectionCallback_(conn);
}

void TcpClient::closeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(connection_ != nullptr);
    connection_.reset();
    connectionCallback_(conn);
}