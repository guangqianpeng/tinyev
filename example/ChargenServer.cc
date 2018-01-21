//
// Created by frank on 17-9-3.
//

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/TcpServer.h>

using namespace ev;

class ChargenServer
{
public:
    ChargenServer(EventLoop* loop, const InetAddress& addr)
            : server_(loop, addr),
              transfered_(0)
    {
        server_.setConnectionCallback(std::bind(
                &ChargenServer::onConnection, this, _1));
        server_.setWriteCompleteCallback(std::bind(
                &ChargenServer::onWriteComplete, this, _1));

        std::string line;
        for (int i = 33; i < 127; ++i)
            line.push_back(char(i));
        line += line;

        for (size_t i = 0; i < 127-33; ++i)
            message_ += line.substr(i, 72) + '\n';
        transfered_ += message_.size();
    }

    void start() { server_.start(); }

    void onConnection(const TcpConnectionPtr& conn)
    {
        INFO("connection %s is [%s]",
             conn->name().c_str(),
             conn->connected() ? "up":"down");

        if (conn->connected()) {
            conn->send("[tinyev chargen server]\n");
            conn->send(message_);
        }
        else {
            INFO("%ld bytes transfered", transfered_);
        }
    }

    void onWriteComplete(const TcpConnectionPtr& conn)
    {
        conn->send(message_);
        transfered_ += message_.size();
    }

private:
    TcpServer server_;
    size_t transfered_;
    std::string message_;
};

int main()
{
    setLogLevel(LOG_LEVEL_TRACE);
    EventLoop loop;
    InetAddress addr(9877);
    ChargenServer server(&loop, addr);
    server.start();
    loop.loop();
}