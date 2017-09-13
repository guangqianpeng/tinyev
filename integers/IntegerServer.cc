//
// Created by frank on 17-9-11.
//

#include <climits>

#include "Logger.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "Codec.h"
#include "TcpConnection.h"

class IntegerServer: noncopyable
{
public:
	IntegerServer(EventLoop* loop, const InetAddress& addr, const std::string& fileName)
			: loop_(loop),
			  server_(loop, addr),
			  file_(fopen(fileName.c_str(), "r"))
	{
		if (file_ == nullptr)
			SYSFATAL("fopen()");

		codec_.setCommandSumCallback(std::bind(
				&IntegerServer::sum, this, _1));
		codec_.setCommandLessCallback(std::bind(
				&IntegerServer::lessEqualCount, this, _1, _2));

		server_.setConnectionCallback(std::bind(
				&IntegerServer::onConnection, this, _1));
		server_.setMessageCallback(std::bind(
				&Codec::parseMessage, &codec_, _1, _2));
	}

	~IntegerServer() { fclose(file_); }

	void start() { server_.start(); }

private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		INFO("connection %s is [%s]",
			 conn->name().c_str(),
			 conn->connected() ? "up":"down");
	}

#define checkForeachError do { \
		if (success) break; \
		WARN("bad file content"); \
		conn->send("bad file content"); \
		conn->shutdown(); \
		return; \
	} while(false)

	void sum(const TcpConnectionPtr& conn)
	{
		__int128 sum = 0;
		int64_t count = 0;

		bool success = foreach(
				[&sum, &count](int64_t elem) {
					sum += elem;
					count++;
				});

		checkForeachError;

		INFO("sum up %ld integers", count);
		codec_.sendAnswerSum(conn, sum, count);
	}

	void lessEqualCount(const TcpConnectionPtr& conn, int64_t target)
	{
		int64_t lessCount = 0;
		int64_t equalCount = 0;

		bool success = foreach(
				[&lessCount, &equalCount, target](int64_t elem) {
					if (elem < target)
						lessCount++;
					else if (elem == target)
						equalCount++;
				});

		checkForeachError;

		INFO("less than %ld: %ld, equal to %ld: %ld");
		codec_.sendAnswerLess(conn, lessCount, equalCount);
	}

#undef checkForeachError

	template <typename Func>
	bool foreach(Func&& func)
	{
		rewind(file_);

		bool empty = true;

		char buf[21];
		while (fscanf(file_, "%20s", buf) == 1) {
			char* end = nullptr;
			int64_t elem = strtol(buf, &end, 10);
			if (end == buf || *end != '\0')
				break;
			if (errno == ERANGE && (elem == LONG_MIN || elem == LONG_MAX))
				break;

			func(elem);
			empty = false;
		}

		return !empty && feof(file_) != 0;
	}

	EventLoop* loop_;
	Codec codec_;
	TcpServer server_;
	FILE* file_;
};

int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("usage: %s filename", argv[0]);
		exit(EXIT_FAILURE);
	}

	setLogLevel(LOG_LEVEL_TRACE);

	EventLoop loop;
	InetAddress addr(9877);
	IntegerServer server(&loop, addr, argv[1]);
	server.start();
	loop.loop();
}