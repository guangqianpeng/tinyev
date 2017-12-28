//
// Created by frank on 17-9-1.
//

#ifndef TINYEV_ACCEPTOR_H
#define TINYEV_ACCEPTOR_H

#include <memory>

#include <tinyev/noncopyable.h>
#include <tinyev/InetAddress.h>
#include <tinyev/Channel.h>

namespace tinyev
{

class EventLoop;

class Acceptor:noncopyable
{
public:
    Acceptor(EventLoop* loop, const InetAddress& local);
    ~Acceptor();


    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

private:
    void handleRead();

    EventLoop* loop_;
    const int acceptFd_;
    Channel acceptChannel_;
    InetAddress local_;
    bool listening_;
    NewConnectionCallback newConnectionCallback_;
};

}



#endif //TINYEV_ACCEPTOR_H
