//
// Created by frank on 17-9-4.
//

#ifndef TINYEV_TCPCLIENT_H
#define TINYEV_TCPCLIENT_H

#include <tinyev/Callbacks.h>
#include <tinyev/Connector.h>
#include <tinyev/Timer.h>

namespace ev
{

class TcpClient: noncopyable
{
public:
    TcpClient(EventLoop* loop, const InetAddress& peer);
    ~TcpClient();

    void start();
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }
    void setErrorCallback(const ErrorCallback& cb)
    { connector_->setErrorCallback(cb); }

private:
    void retry();
    void newConnection(int connfd, const InetAddress& local, const InetAddress& peer);
    void closeConnection(const TcpConnectionPtr& conn);

private:
    typedef std::unique_ptr<Connector> ConnectorPtr;

    EventLoop* loop_;
    bool connected_;
    const InetAddress peer_;
    Timer* retryTimer_;
    ConnectorPtr connector_;
    TcpConnectionPtr connection_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
};

}

#endif //TINYEV_TCPCLIENT_H
