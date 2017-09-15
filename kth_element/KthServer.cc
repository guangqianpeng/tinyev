//
// Created by frank on 17-9-11.
//

#include <climits>
#include <unistd.h> //getpid
#include <algorithm> // sort

#include "Logger.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "Codec.h"
#include "TcpConnection.h"


class KthServer: noncopyable
{
public:
	KthServer(EventLoop* loop, const InetAddress& addr, int64_t count)
			: loop_(loop),
			  server_(loop, addr)
	{
		generate(count, 0, INT32_MAX);

		INFO("count: %ld, min: %ld, max: %ld", count_, min_, max_);

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
		auto lower = std::lower_bound(numbers_.begin(), numbers_.end(), guess);
		auto upper = std::upper_bound(numbers_.begin(), numbers_.end(), guess);

		int64_t lessCount = lower - numbers_.begin();
		int64_t equalCount = upper - lower;

		INFO("guess %ld, less %ld, equal %ld",
			 guess, lessCount, equalCount);
		codec_.sendAnswer(conn, lessCount, equalCount);
	}

	void generate(int64_t count, int64_t min, int64_t max)
	{
		unsigned short xsubi[3];
		xsubi[0] = static_cast<unsigned short>(getpid());
		xsubi[1] = static_cast<unsigned short>(time(nullptr));
		xsubi[2] = 0;

		assert(max >= min);

		int64_t range = max - min;
		for (int64_t i = 0; i < count; ++i) {
			int64_t value = min;
			if (range > 0) {
				value += nrand48(xsubi) % range;
			}
			numbers_.push_back(value);
		}

		std::sort(numbers_.begin(), numbers_.end());

		sum_ = 0;
		for (int64_t i: numbers_)
			sum_ += i;
		count_ = count;
		min_ = numbers_.front();
		max_ = numbers_.back();

#ifndef NDEBUG
		for (int64_t i: numbers_)
			printf("%ld ", i);
		printf("\n");
#endif

	}

	EventLoop* loop_;
	Codec codec_;
	TcpServer server_;
	__int128 sum_;
	int64_t count_;
	int64_t min_, max_;
	std::vector<int64_t> numbers_;
};

void usage()
{
	printf("usage: ./KthServer #count");
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

	EventLoop loop;
	InetAddress addr(9877);
	KthServer server(&loop, addr, count);
	server.start();
	loop.loop();
}