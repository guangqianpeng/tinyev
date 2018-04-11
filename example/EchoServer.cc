//
// Created by frank on 17-9-1.
//

#include <map>

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/TcpServer.h>

using namespace ev;

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, size_t numThread = 1, Nanosecond timeout = 5s)
            : loop_(loop),
              server_(loop, addr),
              numThread_(numThread),
              timeout_(timeout),
              timer_(loop_->runEvery(timeout_, [this](){onTimeout();}))
    {
        server_.setConnectionCallback(std::bind(
                &EchoServer::onConnection, this, _1));
        server_.setMessageCallback(std::bind(
                &EchoServer::onMessage, this, _1, _2));
        server_.setWriteCompleteCallback(std::bind(
                &EchoServer::onWriteComplete, this, _1));
    }

    ~EchoServer()
    { loop_->cancelTimer(timer_); }

    void start()
    {
        // set thread num here
        server_.setNumThread(numThread_);
        server_.start();
    }

    void onConnection(const TcpConnectionPtr& conn)
    {
        INFO("connection %s is [%s]",
             conn->name().c_str(),
             conn->connected() ? "up":"down");

        if (conn->connected()) {
            conn->setHighWaterMarkCallback(
                    std::bind(&EchoServer::onHighWaterMark, this, _1, _2),
                    1024);
            expireAfter(conn, timeout_);
        }
        else connections_.erase(conn);
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
    {
        TRACE("connection %s recv %lu bytes",
             conn->name().c_str(),
             buffer.readableBytes());

        // send will retrieve the buffer
        conn->send(buffer);
        expireAfter(conn, timeout_);
    }

    void onHighWaterMark(const TcpConnectionPtr& conn, size_t mark)
    {
        INFO("high water mark %lu bytes, stop read", mark);
        conn->stopRead();
        expireAfter(conn, 2 * timeout_);
    }

    void onWriteComplete(const TcpConnectionPtr& conn)
    {
        if (!conn->isReading()) {
            INFO("write complete, start read");
            conn->startRead();
            expireAfter(conn, timeout_);
        }
    }

private:
    void expireAfter(const TcpConnectionPtr& conn, Nanosecond interval)
    {
        connections_[conn] = clock::nowAfter(interval);
    }

    void onTimeout()
    {
        for (auto it = connections_.begin(); it != connections_.end(); ) {
            if (it->second <= clock::now()) {
                INFO("connection timeout force close");
                it->first->forceClose();
                it = connections_.erase(it);
            }
            else it++;
        }
    }

private:
    EventLoop* loop_;
    TcpServer server_;
    const size_t numThread_;
    const Nanosecond timeout_;
    Timer* timer_;
    typedef std::map<TcpConnectionPtr, Timestamp> ConnectionList;
    ConnectionList connections_;
};

int main()
{
    setLogLevel(LOG_LEVEL_TRACE);
    EventLoop loop;
    InetAddress addr(9877);
    EchoServer server(&loop, addr, 1, 5s);
    server.start();

    loop.runAfter(10s, [&](){
        int countdown = 5;
        INFO("server quit after %d second...", countdown);
        loop.runEvery(1s, [&, countdown]() mutable {
            INFO("server quit after %d second...", --countdown);
            if (countdown == 0)
                loop.quit();
        });
    });

    loop.loop();
}