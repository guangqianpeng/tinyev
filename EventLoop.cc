//
// Created by frank on 17-8-31.
//

#include <cassert>
#include <sys/eventfd.h>
#include <sys/types.h> // pid_t
#include <unistd.h>	// syscall()
#include <syscall.h> // SYS_gettid

#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"

namespace
{

__thread EventLoop* t_Eventloop = nullptr;

pid_t gettid()
{
	return static_cast<pid_t>(::syscall(SYS_gettid));
}

}

EventLoop::EventLoop()
		: tid_(gettid()),
		  quit_(false),
		  doingPendingTasks_(false),
		  poller_(this),
		  wakeupFd_(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)),
		  wakeupChannel_(this, wakeupFd_)
{
	if (wakeupFd_ == -1)
		SYSFATAL("EventLoop::eventfd()");

	wakeupChannel_.setReadCallback([this](){handleRead();});
	wakeupChannel_.enableRead();

	assert(t_Eventloop == nullptr);
	t_Eventloop = this;
}

EventLoop::~EventLoop()
{
	assert(t_Eventloop == this);
	t_Eventloop = nullptr;
}

void EventLoop::loop()
{
	assertInLoopThread();
	TRACE("EventLoop %p polling", this);
	while (!quit_) {
		activeChannels_.clear();
		poller_.poll(activeChannels_);
		for (auto channel: activeChannels_)
			channel->handleEvents();
		doPendingTasks();
	}
	TRACE("EventLoop %p quit", this);
}

void EventLoop::quit()
{
	assert(!quit_);
	quit_ = true;
	if (!isInLoopThread())
		wakeup();
}

void EventLoop::runInLoop(const Task& task)
{
	if (isInLoopThread())
		task();
	else
		queueInLoop(task);
}

void EventLoop::queueInLoop(const Task& task)
{
	{
		std::lock_guard<std::mutex> guard(mutex_);
		pendingTasks_.push_back(task);
	}
	if (!isInLoopThread() || doingPendingTasks_)
		wakeup();
}

void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
	if (n != sizeof(one))
		SYSERR("EventLoop::wakeup() should ::write() %lu bytes", sizeof(one));
}


void EventLoop::updateChannel(Channel* channel)
{
	assertInLoopThread();
	poller_.updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
	assertInLoopThread();
	channel->disableAll();
}

void EventLoop::assertInLoopThread()
{
	assert(isInLoopThread());
}

bool EventLoop::isInLoopThread()
{
	pid_t x = gettid();
	return tid_ == x;
}

void EventLoop::doPendingTasks()
{
	assertInLoopThread();
	std::vector<Task> tasks;
	{
		std::lock_guard<std::mutex> guard(mutex_);
		tasks.swap(pendingTasks_);
	}
	doingPendingTasks_ = true;
	for (Task& task: tasks)
		task();
	doingPendingTasks_ = false;
}

void EventLoop::handleRead()
{
	uint64_t one;
	ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
	if (n != sizeof(one))
		SYSERR("EventLoop::handleRead() should ::read() %lu bytes", sizeof(one));
}