//
// Created by frank on 17-11-17.
//
#include <sys/timerfd.h>
#include <strings.h>
#include <unistd.h>
#include <ratio> // std::nano::den

#include "Logger.h"
#include "EventLoop.h"
#include "TimerQueue.h"

using namespace tinyev;

namespace
{

int timerfdCreate()
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd == -1)
        SYSFATAL("timer_create()");
    return fd;
}

void timerfdRead(int fd)
{
    uint64_t val;
    ssize_t n = read(fd, &val, sizeof(val));
    assert(n > 0); (void)n;
}

struct timespec durationFromNow(Timestamp when)
{
    struct timespec ret;
    Nanoseconds ns = when - Clock::now();
    if (ns < 1ms) ns = 1ms;

    ret.tv_sec = static_cast<time_t>(ns.count() / std::nano::den);
    ret.tv_nsec = ns.count() % std::nano::den;
    return ret;
}

void timerfdSet(int fd, Timestamp when)
{
    struct itimerspec oldtime, newtime;
    bzero(&oldtime, sizeof(itimerspec));
    bzero(&newtime, sizeof(itimerspec));
    newtime.it_value = durationFromNow(when);

    int ret = timerfd_settime(fd, 0, &newtime, &oldtime);
    if (ret == -1)
        SYSERR("timerfd_settime()");
}

}

TimerQueue::TimerQueue(EventLoop *loop)
        : loop_(loop),
          timerfd_(timerfdCreate()),
          timerChannel_(loop, timerfd_)
{
    loop_->assertInLoopThread();
    timerChannel_.setReadCallback([this](){handleRead();});
    timerChannel_.enableRead();
}

TimerQueue::~TimerQueue()
{
    for (auto& p: timers_)
        delete p.second;
    ::close(timerfd_);
}


void TimerQueue::addTimer(TimerCallback cb, Timestamp when, Nanoseconds interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop([=](){
        auto ret = timers_.insert({when, timer});
        assert(ret.second);

        if (timers_.begin() == ret.first)
            timerfdSet(timerfd_, when);
    });
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();

    Timestamp now(Clock::now());
    for (auto& e: getExpired(now)) {
        Timer* timer = e.second;
        assert(timer->expired(now));

        timer->run();
        if (timer->repeat()) {
            timer->restart();
            e.first = timer->when();
            timers_.insert(e);
        }
        else delete timer;
    }

    timerfdRead(timerfd_);
    if (!timers_.empty())
        timerfdSet(timerfd_, timers_.begin()->first);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    Entry en(now + 1ms, nullptr);
    std::vector<Entry> entries;

    auto end = timers_.lower_bound(en);
    assert(end != timers_.begin());
    entries.assign(timers_.begin(), end);
    timers_.erase(timers_.begin(), end);

    return entries;
}

