//
// Created by frank on 17-9-11.
//

#include <climits>

#include "Logger.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "Codec.h"
#include "TcpConnection.h"

FILE* textToBinary(const std::string& fileName)
{
	FILE* textFile = fopen(fileName.c_str(), "r");
	if (textFile == nullptr)
		return nullptr;

	FILE* binFile = tmpfile();

	INFO("transfer text to binary...");

	bool empty = true;
	char buf[21];
	while (fscanf(textFile, "%20s", buf) == 1) {
		char* end = nullptr;
		int64_t elem = strtol(buf, &end, 10);

		if (end == buf || *end != '\0')
			break;
		if (errno == ERANGE && (elem == LONG_MIN || elem == LONG_MAX))
			break;

		size_t n = fwrite(&elem, sizeof(elem), 1, binFile);
		if (n != 1)
			break;

		empty = false;
	}

	if (empty || feof(textFile) == 0) {
		fclose(binFile);
		return nullptr;
	}

	fclose(textFile);

	INFO("transfer done");

	return binFile;
}

class KthServer: noncopyable
{
public:
	KthServer(EventLoop* loop, const InetAddress& addr, const std::string& fileName)
			: loop_(loop),
			  server_(loop, addr),
			  file_(textToBinary(fileName)),
			  sum_(0),
			  count_(0),
			  min_(INT64_MAX),
			  max_(INT64_MIN)
	{
		if (file_ == nullptr)
			SYSFATAL("textToBinary()");

		foreach([this](int64_t elem) {
			if (min_ > elem) min_ = elem;
			if (max_ < elem) max_ = elem;
			sum_ += elem;
			count_++;
		});

		INFO("count: %ld, min: %ld, max: %ld", count_, min_, max_);

		codec_.setQueryCallback(std::bind(
				&KthServer::lessEqualCount, this, _1, _2));
		server_.setConnectionCallback(std::bind(
				&KthServer::onConnection, this, _1));
		server_.setMessageCallback(std::bind(
				&Codec::parseMessage, &codec_, _1, _2));
	}

	~KthServer() { fclose(file_); }

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
		int64_t lessCount = 0;
		int64_t equalCount = 0;

		INFO("guess %ld...", guess);

		foreach([&lessCount, &equalCount, guess](int64_t elem) {
					if (elem < guess)
						lessCount++;
					else if (elem == guess)
						equalCount++;
				});

		INFO("guess %ld, less %ld, equal %ld",
			 guess, lessCount, equalCount);
		codec_.sendAnswer(conn, lessCount, equalCount);
	}

	template <typename Func>
	void foreach(Func&& func) // universal reference
	{
		rewind(file_);

		const int bufSize = 8192;
		int64_t buf[bufSize]; // 8192*8=64KB
		while (true) {
			size_t n = fread(buf, sizeof(int64_t), bufSize, file_);
			for (size_t i = 0; i < n ; ++i)
				func(buf[i]);
			if (n < bufSize)
				break;
		}

		assert(feof(file_) != 0);
	}

	EventLoop* loop_;
	Codec codec_;
	TcpServer server_;
	FILE* file_;
	__int128 sum_;
	int64_t count_;
	int64_t min_, max_;
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
	KthServer server(&loop, addr, argv[1]);
	server.start();
	loop.loop();
}