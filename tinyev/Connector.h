//
// Created by frank on 17-9-4.
//

#ifndef TINYEV_CONNECTOR_H
#define TINYEV_CONNECTOR_H

#include <functional>

#include <tinyev/InetAddress.h>
#include <tinyev/Channel.h>
#include <tinyev/noncopyable.h>

namespace ev
{

class EventLoop;
class InetAddress;

class Connector: noncopyable
{
public:
    Connector(EventLoop* loop, const InetAddress& peer);
    ~Connector();

    void start();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    void setErrorCallback(const ErrorCallback& cb)
    { errorCallback_ = cb; }

private:
    void handleWrite();

    EventLoop* loop_;
    const InetAddress peer_;
    const int sockfd_;
    bool connected_;
    bool started_;
    Channel channel_;
    NewConnectionCallback newConnectionCallback_;
    ErrorCallback errorCallback_;
};

}


#endif //TINYEV_CONNECTOR_H
