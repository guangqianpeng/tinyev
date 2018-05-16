//
// Created by frank on 17-9-15.
//

#include <cassert>

#include <tinyev/EventLoopThread.h>
#include <tinyev/EventLoop.h>

using namespace ev;

EventLoopThread::~EventLoopThread()
{
    if (started_) {
        if (loop_ != nullptr) {
            loop_->quit();
        }
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!started_);
    started_ = true;

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
