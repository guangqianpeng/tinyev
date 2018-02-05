//
// Created by frank on 17-9-11.
//

#include <climits>
#include <unistd.h> //getpid
#include <algorithm> // sort

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpServer.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/EventLoopThread.h>

#include "Codec.h"

using namespace ev;

class KthServer: noncopyable
{
public:
	KthServer(EventLoop* loop, const InetAddress& addr, int64_t count, int nThreads)
			: ioLoop_(loop),
			  nThreads_(nThreads),
			  server_(loop, addr),
			  sum_(0),
              count_(count),
			  min_(INT64_MAX),
			  max_(INT64_MIN)
	{
		if (count <= 0)
			FATAL("bad count");
		if (nThreads <= 0)
			FATAL("bad thread number");

		startWorkerThreads();

		INFO("generate numbers...");
		generateNumbers(count, 0, INT32_MAX);

		INFO("sort in threads...");
		runInThreads([this](std::vector<int64_t>& vec) {
			std::sort(vec.begin(), vec.end());
		});

		for (auto& vec: numbers_) {
			(void)vec;
			assert(std::is_sorted(vec.begin(), vec.end()));
		}
		INFO("count: %ld, sum: %s, min: %ld, max: %ld",
			 count_, toString(sum_).c_str(), min_, max_);

		codec_.setQueryCallback(std::bind(
				&KthServer::lessEqualCount, this, _1, _2));
		server_.setConnectionCallback(std::bind(
				&KthServer::onConnection, this, _1));
		server_.setMessageCallback(std::bind(
				&Codec::parseMessage, &codec_, _1, _2));
	}

	void start() { server_.start(); }

private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		INFO("connection %s is [%s]",
			 conn->name().c_str(),
			 conn->connected() ? "up":"down");
		if (conn->connected())
			codec_.sendState(conn, sum_, count_, min_, max_);
	}

	void lessEqualCount(const TcpConnectionPtr& conn, int64_t guess)
	{
		std::atomic<int64_t> lessCount(0);
		std::atomic<int64_t> equalCount(0);

		// blocking calls
		runInThreads([this, guess, &lessCount, &equalCount]
							 (std::vector<int64_t>& vec){
			auto lower = std::lower_bound(vec.begin(), vec.end(), guess);
			auto upper = std::upper_bound(vec.begin(), vec.end(), guess);
			int64_t t_lessCount = lower - vec.begin();
			int64_t t_equalCount = upper - lower;
			lessCount += t_lessCount;
			equalCount += t_equalCount;
		});

		INFO("guess %ld, less %ld, equal %ld",
			 guess, int64_t(lessCount), int64_t(equalCount));
		codec_.sendAnswer(conn, lessCount, equalCount);
	}

	void startWorkerThreads()
	{
		for (int i = 0; i < nThreads_; ++i) {
			auto thread = new EventLoopThread();
			auto loop = thread->startLoop();
			threads_.emplace_back(thread);
			loops_.push_back(loop);
		}

		INFO("%d threads started", nThreads_);
	}

	void generateNumbers(int64_t count, int64_t min, int64_t max)
	{
		unsigned short xsubi[3];
		xsubi[0] = static_cast<unsigned short>(getpid());
		xsubi[1] = static_cast<unsigned short>(time(nullptr));
		xsubi[2] = 0;

		assert(max >= min);

		int64_t range = max - min;
		int64_t vecSize = count / nThreads_;

		numbers_.resize(nThreads_);
		for (auto& vec: numbers_) {
			vec.reserve(vecSize + nThreads_);
			for (int i = 0; i < vecSize; ++i) {
				vec.push_back(generateOne(min, range, xsubi));
			}
		}

		int64_t remain = count - vecSize * nThreads_;
		while (--remain >= 0)
			numbers_[0].push_back(generateOne(min, range, xsubi));
	}

	int64_t generateOne(int64_t min, int64_t range, unsigned short* xsubi)
	{
		int64_t value = min;
		if (range > 0) {
			value += nrand48(xsubi) % range;
		}
		sum_ += value;
		if (min_ > value) min_ = value;
		if (max_ < value) max_ = value;

		return value;
	}

	template <typename Func>
	void runInThreads(Func&& func)
	{
		CountDownLatch latch(nThreads_);
		for (int i = 0; i < nThreads_; ++i) {
			loops_[i]->assertNotInLoopThread();
			loops_[i]->queueInLoop([&latch, &func, i, this](){
				loops_[i]->assertInLoopThread();
				func(numbers_[i]);
				latch.count();
			});
		}
		latch.wait();
	}

	typedef std::vector<EventLoop*> WokerLoops;
	typedef std::unique_ptr<EventLoopThread> ThreadPtr;
	typedef std::vector<ThreadPtr> ThreadPtrList;
	typedef std::vector<std::vector<int64_t>> Numbers;

	EventLoop* ioLoop_;

	WokerLoops loops_;
	ThreadPtrList threads_;
	Numbers numbers_;
	const int nThreads_;

	Codec codec_;
	TcpServer server_;
	__int128 sum_;
	int64_t count_;
	int64_t min_, max_;
};

void usage()
{
	printf("usage: ./KthServer #count [#thread]\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
	if (argc < 2)
		usage();

	setLogLevel(LOG_LEVEL_TRACE);

	char* end = nullptr;
	int64_t count = strtol(argv[1], &end, 10);
	if (*end != '\0')
		usage();

	size_t nThreads = std::thread::hardware_concurrency();
	if (argc > 2) {
		nThreads = strtoul(argv[2], &end, 10);
		if (*end != '\0')
			usage();
	}

	EventLoop loop;
	InetAddress addr(9877);
	KthServer server(&loop, addr, count, static_cast<int>(nThreads));
	server.start();
	loop.loop();
}