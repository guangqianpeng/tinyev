//
// Created by frank on 17-9-15.
//

#ifndef TINYEV_EVENTLOOPTHREAD_H
#define TINYEV_EVENTLOOPTHREAD_H

#include <thread>

#include <tinyev/CountDownLatch.h>

namespace ev
{

class EventLoop;

class EventLoopThread: noncopyable
{
public:
    EventLoopThread() = default;
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void runInThread();

    bool started_ = false;
    EventLoop* loop_ = nullptr;
    std::thread thread_;
    CountDownLatch latch_{1};
};

}

#endif //TINYEV_EVENTLOOPTHREAD_H
