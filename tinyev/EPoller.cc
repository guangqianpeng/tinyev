//
// Created by frank on 17-8-31.
//

#include <unistd.h>
#include <sys/epoll.h>
#include <cassert>

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>

using namespace ev;

EPoller::EPoller(EventLoop* loop)
        :loop_(loop),
         events_(128),
         epollfd_(::epoll_create1(EPOLL_CLOEXEC))
{
    if (epollfd_ == -1)
        SYSFATAL("EPoller::epoll_create1()");
}

EPoller::~EPoller()
{
    ::close(epollfd_);
}

void EPoller::poll(ChannelList& activeChannels)
{
    loop_->assertInLoopThread();
    int maxEvents = static_cast<int>(events_.size());
    int nEvents = epoll_wait(epollfd_, events_.data(), maxEvents, -1);
    if (nEvents == -1) {
        if (errno != EINTR)
            SYSERR("EPoller::epoll_wait()");
    }
    else if (nEvents > 0) {
        for (int i = 0; i < nEvents; ++i) {
            auto channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->setRevents(events_[i].events);
            activeChannels.push_back(channel);
        }
        if (nEvents == maxEvents)
            events_.resize(2 * events_.size());
    }
}

void EPoller::updateChannel(Channel* channel)
{
    loop_->assertInLoopThread();
    int op = 0;
    if (!channel->polling) {
        assert(!channel->isNoneEvents());
        op = EPOLL_CTL_ADD;
        channel->polling = true;
    }
    else if (!channel->isNoneEvents()) {
        op = EPOLL_CTL_MOD;
    }
    else {
        op = EPOLL_CTL_DEL;
        channel->polling = false;
    }
    updateChannel(op, channel);
}

void EPoller::updateChannel(int op, Channel* channel)
{
    struct epoll_event ee;
    ee.events = channel->events();
    ee.data.ptr = channel;
    int ret = ::epoll_ctl(epollfd_, op, channel->fd(), &ee);
    if (ret == -1)
        SYSERR("EPoller::epoll_ctl()");
}

