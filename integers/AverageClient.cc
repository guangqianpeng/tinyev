//
// Created by frank on 17-9-13.
//

#include <unordered_set>

#include "Logger.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpClient.h"
#include "Codec.h"

class AverageClient: noncopyable
{
public:
	AverageClient(EventLoop* loop, const std::vector<InetAddress>& peers)
			: loop_(loop),
			  sum_(0),
			  count_(0),
			  totalResponse_(0)
	{
		for (auto& peer: peers) {
			auto client = new TcpClient(loop, peer);
			client->setConnectionCallback(std::bind(
					&AverageClient::onConnection, this, _1));
			client->setMessageCallback(std::bind(
					&Codec::parseMessage, &codec_, _1, _2));
			client->setErrorCallback(std::bind(
					&AverageClient::onConnectError, this));
			clients_.emplace_back(client);
		}

		codec_.setAnswerSumCallback(std::bind(
				&AverageClient::onMessage, this, _1, _2, _3));
	}

	void start()
	{
		for (auto& client: clients_)
			client->start();
	}

private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		INFO("connection %s is [%s]",
			 conn->name().c_str(),
			 conn->connected() ? "up":"down");
		if (conn->connected()) {
			connections_.insert(conn);
			codec_.sendCommandSum(conn);
		}
		else {
			connections_.erase(conn);
			onConnectError();
		}
	}

	void onConnectError()
	{
		if (connections_.empty()) {
			INFO("all connections closed, now quit");
			loop_->quit();
		}
		else {
			auto it = connections_.begin();
			(*it)->shutdown();
		}
	}

	void onMessage(const TcpConnectionPtr& conn, __int128 sum, int64_t count)
	{
		sum_ += sum;
		count_ += count;
		totalResponse_++;
		if (totalResponse_ == clients_.size()) {
			auto avg = static_cast<int64_t>(sum_/count_);
			INFO("total %ld numbers, average %ld", count_, avg);
			conn->shutdown();
		}
	}

	typedef std::unique_ptr<TcpClient> TcpClientPtr;
	typedef std::vector<TcpClientPtr> TcpClientList;
	typedef std::unordered_set<TcpConnectionPtr> ConnectionSet;

	EventLoop* loop_;
	Codec codec_;
	TcpClientList clients_;
	ConnectionSet connections_;
	__int128 sum_;
	int64_t count_;
	size_t totalResponse_;
};

void usage()
{
	printf("usage: ip port...\n");
	exit(EXIT_FAILURE);
}

std::vector<InetAddress> parseArgs(int argc, char** argv)
{
	std::vector<InetAddress> peers;

	if (argc <= 2 || argc % 2 != 1)
		usage();
	for (int i = 1; i < argc; i += 2) {
		const char* ip = argv[i];
		auto port = static_cast<uint16_t>(atoi(argv[i+1]));
		peers.emplace_back(ip, port);
	}

	return peers;
}

int main(int argc, char** argv)
{
//	setLogLevel(LOG_LEVEL_TRACE);

	EventLoop loop;
	std::vector<InetAddress> peers = parseArgs(argc, argv);
	AverageClient client(&loop, peers);
	client.start();
	loop.loop();
}