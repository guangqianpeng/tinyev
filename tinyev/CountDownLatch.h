//
// Created by frank on 17-9-15.
//

#ifndef TINYEV_COUNTDOWNLATCH_H
#define TINYEV_COUNTDOWNLATCH_H

#include <mutex>
#include <condition_variable>

#include <tinyev/noncopyable.h>

namespace ev
{

class CountDownLatch: noncopyable
{
public:
    explicit
    CountDownLatch(int count)
            : count_(count)
    {}

    void count()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        count_--;
        if (count_ <= 0)
            cond_.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (count_ > 0)
            cond_.wait(lock);
    }

private:
    int count_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

}


#endif //TINYEV_COUNTDOWNLATCH_H
