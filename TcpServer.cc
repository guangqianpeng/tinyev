//
// Created by frank on 17-9-1.
//

#include "Logger.h"
#include "TcpConnection.h"
#include "TcpServerSingle.h"
#include "EventLoop.h"
#include "TcpServer.h"

TcpServer::TcpServer(EventLoop* loop, const InetAddress& local)
		: baseLoop_(loop),
		  numThreads_(0),
		  started_(false),
		  local_(local),
		  threadInitCallback_(defaultThreadInitCallback),
		  connectionCallback_(defaultConnectionCallback),
		  messageCallback_(defaultMessageCallback)
{
	TRACE("TcpServer() %s", local.toIpPort().c_str());
}

TcpServer::~TcpServer()
{
	for (auto& loop: eventLoops_)
		if (loop != nullptr)
			loop->quit();
	for (auto& thread: threads_)
		thread->join();
	TRACE("~TcpServer()");
}

void TcpServer::setNumThread(size_t n)
{
	baseLoop_->assertInLoopThread();
	assert(!started_);
	numThreads_ = n;
	eventLoops_.resize(n);
}

void TcpServer::start()
{
	baseLoop_->assertInLoopThread();
	assert(!started_);
	started_ = true;

	INFO("TcpServer::start() %s with %lu threads",
		 local_.toIpPort().c_str(), numThreads_);

	for (size_t i = 0; i < numThreads_; ++i) {
		auto thread = new std::thread(std::bind(
				&TcpServer::runInThread, this, i));
		{
			std::unique_lock<std::mutex> lock(mutex_);
			while (eventLoops_[i] == nullptr)
				cond_.wait(lock);
		}
		threads_.emplace_back(thread);
	}

	baseServer_ = std::make_unique<TcpServerSingle>(baseLoop_, local_);
	baseServer_->setConnectionCallback(connectionCallback_);
	baseServer_->setMessageCallback(messageCallback_);
	baseServer_->setWriteCompleteCallback(writeCompleteCallback_);
	threadInitCallback_(0);
}

void TcpServer::runInThread(size_t index)
{
	EventLoop loop;
	TcpServerSingle server(&loop, local_);

	server.setConnectionCallback(connectionCallback_);
	server.setMessageCallback(messageCallback_);
	server.setWriteCompleteCallback(writeCompleteCallback_);

	{
		std::lock_guard<std::mutex> guard(mutex_);
		eventLoops_[index] = &loop;
		cond_.notify_one();
	}

	threadInitCallback_(index + 1);
	loop.loop();
	eventLoops_[index] = nullptr;
}
