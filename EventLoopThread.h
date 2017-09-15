//
// Created by frank on 17-9-15.
//

#ifndef TINYEV_EVENTLOOPTHREAD_H
#define TINYEV_EVENTLOOPTHREAD_H

#include <thread>

#include "CountDownLatch.h"
#include "noncopyable.h"

class EventLoop;

class EventLoopThread: noncopyable
{
public:
	EventLoopThread();
	~EventLoopThread();

	EventLoop* startLoop();

private:
	void runInThread();


	EventLoop* loop_;
	std::thread thread_;
	CountDownLatch latch_;
};


#endif //TINYEV_EVENTLOOPTHREAD_H
