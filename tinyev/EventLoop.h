//
// Created by frank on 17-8-31.
//

#ifndef TINYEV_EVENTLOOP_H
#define TINYEV_EVENTLOOP_H

#include <atomic>
#include <mutex>
#include <vector>
#include <sys/types.h>

#include <tinyev/Timer.h>
#include <tinyev/EPoller.h>
#include <tinyev/TimerQueue.h>

namespace tinyev
{

class EventLoop: noncopyable
{
public:

    EventLoop();
    ~EventLoop();

    void loop();
    void quit(); // thread safe

    void runInLoop(const Task& task);
    void queueInLoop(const Task& task);

    Timer* runAt(Timestamp when, TimerCallback callback);
    Timer* runAfter(Nanosecond interval, TimerCallback callback);
    Timer* runEvery(Nanosecond interval, TimerCallback callback);
    void cancelTimer(Timer* timer);

    void wakeup();

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    void assertInLoopThread();
    void assertNotInLoopThread();
    bool isInLoopThread();

private:
    void doPendingTasks();
    void handleRead();
    pid_t tid_;
    bool quit_;
    bool doingPendingTasks_;
    EPoller poller_;
    EPoller::ChannelList activeChannels_;
    int wakeupFd_;
    Channel wakeupChannel_;
    std::mutex mutex_;
    std::vector<Task> pendingTasks_;
    TimerQueue timerQueue_;
};

}

#endif //TINYEV_EVENTLOOP_H
