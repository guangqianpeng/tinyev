//
// Created by frank on 17-9-3.
//

#ifndef TINYEV_THREADPOOL_H
#define TINYEV_THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <vector>
#include <deque>

#include <tinyev/noncopyable.h>
#include <tinyev/Callbacks.h>

namespace ev
{

class ThreadPool: noncopyable
{
public:
    explicit
    ThreadPool(size_t numThread,
               size_t maxQueueSize = 65536,
               const ThreadInitCallback& cb = nullptr);
    ~ThreadPool();

    void runTask(const Task& task);
    void runTask(Task&& task);
    void stop();
    size_t numThreads() const
    { return threads_.size(); }

private:
    void runInThread(size_t index);
    Task take();

    typedef std::unique_ptr<std::thread> ThreadPtr;
    typedef std::vector<ThreadPtr> ThreadList;

    ThreadList threads_;
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::deque<Task> taskQueue_;
    const size_t maxQueueSize_;
    std::atomic_bool running_;
    ThreadInitCallback threadInitCallback_;
};

}

#endif //TINYEV_THREADPOOL_H
