//
// Created by frank on 17-9-15.
//

#include <cassert>

#include "EventLoopThread.h"
#include "EventLoop.h"

using namespace tinyev;

EventLoopThread::EventLoopThread()
        : loop_(nullptr),
          latch_(1)
{}

EventLoopThread::~EventLoopThread()
{
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    assert(loop_ == nullptr);
    thread_ = std::thread([this](){runInThread();});
    latch_.wait();
    assert(loop_ != nullptr);
    return loop_;
}

void EventLoopThread::runInThread()
{
    EventLoop loop;
    loop_ = &loop;
    latch_.count();
    loop.loop();
    loop_ = nullptr;
}
