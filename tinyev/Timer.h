//
// Created by frank on 17-11-17.
//

#ifndef TINYEV_TIMER_H
#define TINYEV_TIMER_H

#include <cassert>

#include <tinyev/Callbacks.h>
#include <tinyev/Channel.h>
#include <tinyev/Timestamp.h>

namespace ev
{

class Timer: noncopyable
{
public:
    Timer(TimerCallback callback, Timestamp when, Nanosecond interval)
            : callback_(std::move(callback)),
              when_(when),
              interval_(interval),
              repeat_(interval_ > Nanosecond::zero()),
              canceled_(false)
    {
    }

    void run() { if (callback_) callback_(); }
    bool repeat() const { return repeat_; }
    bool expired(Timestamp now) const { return now >= when_; }
    Timestamp when() const { return when_; }
    void restart()
    {
        assert(repeat_);
        when_ += interval_;
    }
    void cancel()
    {
        assert(!canceled_);
        canceled_ = true;
    }
    bool canceled() const { return canceled_; }

private:
    TimerCallback callback_;
    Timestamp when_;
    const Nanosecond interval_;
    bool repeat_;
    bool canceled_;
};

}
#endif //TINYEV_TIMER_H
