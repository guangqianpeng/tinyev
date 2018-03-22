//
// Created by frank on 17-8-31.
//

#include <cassert>

#include <tinyev/EventLoop.h>
#include <tinyev/Channel.h>

using namespace ev;

Channel::Channel(EventLoop* loop, int fd)
        : polling(false),
          loop_(loop),
          fd_(fd),
          tied_(false),
          events_(0),
          revents_(0),
          handlingEvents_(false)
{}

Channel::~Channel()
{ assert(!handlingEvents_); }

void Channel::handleEvents()
{
    loop_->assertInLoopThread();
    // channel is always a member of another object
    // e.g. Timer, Acceptor, TcpConnection
    // TcpConnection is managed by std::shared_ptr,
    // and may be destructed when handling events,
    // so we use weak_ptr->shared_ptr to
    // extend it's life-time.
    if (tied_) {
        auto guard = tie_.lock();
        if (guard != nullptr)
            handleEventsWithGuard();
    }
    else handleEventsWithGuard();
}

void Channel::handleEventsWithGuard()
{
    handlingEvents_ = true;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) readCallback_();
    }
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }
    handlingEvents_ = false;
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::remove()
{
    assert(polling);
    loop_->removeChannel(this);
}
