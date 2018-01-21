//
// Created by frank on 17-9-2.
//

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/TcpServer.h>

using namespace ev;

class DiscardServer
{
public:
    DiscardServer(EventLoop* loop, const InetAddress& addr)
            : server(loop, addr)
    {
        server.setMessageCallback(std::bind(
                &DiscardServer::onMessage, this, _1, _2));
    }

    void start()
    { server.start(); }

    void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
    {
        TRACE("connection %s recv %lu bytes",
              conn->name().c_str(),
              buffer.readableBytes());
        buffer.retrieveAll();
    }

private:
    TcpServer server;
};

int main()
{
    //setLogLevel(LOG_LEVEL_TRACE);
    EventLoop loop;
    InetAddress addr(9877);
    DiscardServer server(&loop, addr);
    server.start();
    loop.loop();
}
