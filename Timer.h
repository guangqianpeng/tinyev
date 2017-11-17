//
// Created by frank on 17-11-17.
//

#ifndef TINYEV_TIMER_H
#define TINYEV_TIMER_H

#include <cassert>

#include "Callbacks.h"
#include "Channel.h"
#include "Timestamp.h"

namespace tinyev
{

class Timer: noncopyable
{
public:
    Timer(TimerCallback callback, Timestamp when, Nanoseconds interval)
            : callback_(std::move(callback)),
              when_(when),
              interval_(interval),
              repeat_(interval_ > Nanoseconds::zero())
    {
    }

    void run() { callback_(); }
    bool repeat() const { return repeat_; }
    bool expired(Timestamp now) const { return now >= when_; }
    Timestamp when() const { return when_; }
    void restart()
    {
        assert(repeat_);
        when_ += interval_;
    }

private:
    TimerCallback callback_;
    Timestamp when_;
    Nanoseconds interval_;
    const bool repeat_;
};

}
#endif //TINYEV_TIMER_H
