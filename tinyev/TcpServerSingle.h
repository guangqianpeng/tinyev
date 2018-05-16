//
// Created by frank on 17-9-1.
//

#ifndef TINYEV_TCPSERVERTHREAD_H
#define TINYEV_TCPSERVERTHREAD_H

#include <unordered_set>

#include <tinyev/Callbacks.h>
#include <tinyev/Acceptor.h>

namespace ev
{

class EventLoop;
class TcpServerSingle : noncopyable
{
public:
    TcpServerSingle(EventLoop *loop, const InetAddress &local);

    void setConnectionCallback(const ConnectionCallback &cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    { writeCompleteCallback_ = cb; }

    void start();

private:
    void newConnection(int connfd, const InetAddress &local, const InetAddress &peer);

    void closeConnection(const TcpConnectionPtr &conn);

    typedef std::unordered_set<TcpConnectionPtr> ConnectionSet;

    EventLoop *loop_;
    Acceptor acceptor_;
    ConnectionSet connections_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
};

}

#endif //TINYEV_TCPSERVERTHREAD_H
