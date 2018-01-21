//
// Created by frank on 17-9-1.
//

#ifndef TINYEV_TCPCONNECTION_H
#define TINYEV_TCPCONNECTION_H

#include <any>

#include <tinyev/noncopyable.h>
#include <tinyev/Buffer.h>
#include <tinyev/Callbacks.h>
#include <tinyev/Channel.h>
#include <tinyev/InetAddress.h>

namespace ev
{

class EventLoop;

class TcpConnection: noncopyable,
                     public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, int sockfd,
                  const InetAddress& local,
                  const InetAddress& peer);
    ~TcpConnection();

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = mark; }

    // internal use
    void setCloseCallBack(const CloseCallback& cb)
    { closeCallback_ = cb; }

    // TcpServerSingle
    void connectEstablished();

    bool connected() const;
    bool disconnected() const;

    const InetAddress& local() const
    { return local_; }
    const InetAddress& peer() const
    { return peer_; }

    std::string name() const
    { return peer_.toIpPort() + " -> " + local_.toIpPort(); }

    void setContext(const std::any& context)
    { context_ = context; }
    const std::any& getContext() const
    { return context_; }
    std::any& getContext()
    { return context_; }

    // I/O operations are thread safe
    void send(std::string_view data);
    void send(const char* data, size_t len);
    void send(Buffer& buffer);
    void shutdown();
    void forceClose();

    void stopRead();
    void startRead();
    bool isReading() // not thread safe
    { return channel_.isReading(); };

    const Buffer& inputBuffer() const { return inputBuffer_; }
    const Buffer& outputBuffer() const { return outputBuffer_; }

private:
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const char* data, size_t len);
    void sendInLoop(const std::string& message);
    void shutdownInLoop();
    void forceCloseInLoop();

    int stateAtomicGetAndSet(int newState);

    EventLoop* loop_;
    const int sockfd_;
    Channel channel_;
    int state_;
    InetAddress local_;
    InetAddress peer_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    size_t highWaterMark_;
    std::any context_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
};

}

#endif //TINYEV_TCPCONNECTION_H
