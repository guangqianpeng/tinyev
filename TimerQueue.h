//
// Created by frank on 17-11-17.
//

#ifndef TINYEV_TIMERQUEUE_H
#define TINYEV_TIMERQUEUE_H

#include <memory>
#include <set>

#include "Timer.h"
#include "Channel.h"

namespace tinyev
{

class TimerQueue: noncopyable
{
public:
    explicit
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    Timer* addTimer(TimerCallback cb, Timestamp when, Nanosecond interval);
    void cancelTimer(Timer* timer);

private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;

    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);

private:
    EventLoop* loop_;
    const int timerfd_;
    Channel timerChannel_;
    TimerList timers_;
};

}
#endif //TINYEV_TIMERQUEUE_H
