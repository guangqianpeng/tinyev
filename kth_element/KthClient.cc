//
// Created by frank on 17-9-14.
//

#include <unordered_set>

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/TcpClient.h>

#include "Codec.h"

using namespace ev;

class KthClient: noncopyable
{
public:
	KthClient(EventLoop* loop, const std::vector<InetAddress>& peers, int64_t kth)
			: loop_(loop),
			  kth_(kth),
			  sum_(0),
			  totalCount_(0),
			  min_(INT64_MAX),
			  max_(INT64_MIN),
			  nServerState_(0),
			  nLess_(0),
			  nEqual_(0),
			  nAnsewered_(0),
			  nIteration_(0)
	{
		for (auto& peer: peers) {
			auto client = new TcpClient(loop, peer);
			client->setConnectionCallback(std::bind(
					&KthClient::onConnection, this, _1));
			client->setMessageCallback(std::bind(
					&Codec::parseMessage, &codec_, _1, _2));
			client->setErrorCallback(std::bind(
					&KthClient::onConnectError, this));
			clients_.emplace_back(client);
		}

		codec_.setAnswerCallback(std::bind(
				&KthClient::onAnswer, this, _1, _2, _3));
		codec_.setTellStateCallback(std::bind(
				&KthClient::onServerState, this, _1, _2, _3, _4, _5));
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

	void onServerState(const TcpConnectionPtr& conn,
					   __int128 sum, int64_t count, int64_t min, int64_t max)
	{
		sum_ += sum;
		totalCount_ += count;
		if (min_ > min) min_ = min;
		if (max_ < max) max_ = max;

		INFO("server %s sum: %ld, count: %ld, min: %ld, max: %ld",
			 conn->peer().toIpPort().c_str(), static_cast<int64_t>(sum), count, min, max);

		nServerState_++;
		if (allToldState()) {
			low_ = min_;
			high_ = max_;
			guess_ = low_ + static_cast<uint64_t>(high_ - low_) / 2;

			INFO("global sum: %ld, count: %ld,"
						 " average: %s, min: %ld, max: %ld",
				 static_cast<int64_t>(sum_),
				 totalCount_,
				 toString(sum_ / totalCount_).c_str(),
				 min_, max_);

			if (kth_ <= 0)
				kth_ = (totalCount_ + 1) / 2;
			else if (kth_ > totalCount_)
				kth_ = totalCount_;

			INFO("***start finding %ldth element...", kth_);

			for (auto& c: connections_)
				codec_.sendQuery(c, guess_);
		}
	}

	void onAnswer(const TcpConnectionPtr &conn,
				  int64_t nLess, int64_t nEqual)
	{
		nLess_ += nLess;
		nEqual_ += nEqual;
		nAnsewered_++;

		if (allAnswered()) {
			bool done = false;
			if (nLess_ < kth_ ) {
				if (nLess_ + nEqual_ >= kth_)
					done = true;
				else
					low_ = guess_ + 1;
			}
			else {
				high_ = guess_ - 1;
			}

			nIteration_++;

			assert(nIteration_ <= 64);
			assert(low_ <= high_);

			INFO("iteration %ld, guess %ld, less %ld, euqal %ld",
				 nIteration_, guess_, nLess_, nEqual_);

			if (done) {
				INFO("***find %ldth element: %ld", kth_, guess_);
				conn->shutdown();
			}
			else {
				nLess_ = 0;
				nEqual_ = 0;
				nAnsewered_ = 0;
				guess_ = low_ + (high_ - low_) / 2;
				for (auto& c: connections_)
					codec_.sendQuery(c, guess_);
			}
		}
	}

	bool allToldState() { return nServerState_ == clients_.size(); }
	bool allAnswered() { return nAnsewered_ == clients_.size(); }

	typedef std::unique_ptr<TcpClient> TcpClientPtr;
	typedef std::vector<TcpClientPtr> TcpClientList;
	typedef std::unordered_set<TcpConnectionPtr> ConnectionSet;

	EventLoop* loop_;
	Codec codec_;
	TcpClientList clients_;
	ConnectionSet connections_;

	int64_t kth_;

	// server state
	__int128 sum_;
	int64_t totalCount_;
	int64_t min_, max_;
	size_t nServerState_;

	// for bisection search
	int64_t low_, high_, guess_;
	int64_t nLess_;
	int64_t nEqual_;
	size_t nAnsewered_;
	size_t nIteration_;
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
	setLogLevel(LOG_LEVEL_TRACE);

	EventLoop loop;
	std::vector<InetAddress> peers = parseArgs(argc, argv);
	KthClient client(&loop, peers, -1);
	client.start();
	loop.loop();
}