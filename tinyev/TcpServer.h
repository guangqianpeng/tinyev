//
// Created by frank on 17-9-1.
//

#ifndef TINYEV_TCPSERVER_H
#define TINYEV_TCPSERVER_H

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <tinyev/TcpServerSingle.h>
#include <tinyev/InetAddress.h>
#include <tinyev/Callbacks.h>
#include <tinyev/noncopyable.h>

namespace ev
{

class EventLoopThread;
class TcpServerSingle;
class EventLoop;
class InetAddress;

class TcpServer: noncopyable
{
public:

    TcpServer(EventLoop* loop, const InetAddress& local);
    ~TcpServer();
    // n == 0 || n == 1: all things run in baseLoop thread
    // n > 1: set another (n - 1) eventLoop threads.
    void setNumThread(size_t n);
    // set all threads begin to loop and accept new connections
    // except the baseLoop thread
    void start();

    void setThreadInitCallback(const ThreadInitCallback& cb)
    { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

private:
    void startInLoop();
    void runInThread(size_t index);

    typedef std::unique_ptr<std::thread> ThreadPtr;
    typedef std::vector<ThreadPtr> ThreadPtrList;
    typedef std::unique_ptr<TcpServerSingle> TcpServerSinglePtr;
    typedef std::vector<EventLoop*> EventLoopList;

    EventLoop* baseLoop_;
    TcpServerSinglePtr baseServer_;
    ThreadPtrList threads_;
    EventLoopList eventLoops_;
    size_t numThreads_;
    std::atomic_bool started_;
    InetAddress local_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback threadInitCallback_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
};

}

#endif //TINYEV_TCPSERVER_H
