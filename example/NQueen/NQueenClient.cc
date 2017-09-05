//
// Created by frank on 17-9-4.
//

#include "Logger.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpClient.h"

#include <sstream>
#include <string>

class NQueenClient: noncopyable
{
public:
	typedef std::vector<TcpConnectionPtr> TcpConnectionList;
	typedef std::function<void(const TcpConnectionList&)> AllConnectedCallback;
	typedef std::function<void(const TcpConnectionPtr&, int64_t)> ResponseCallback;

	NQueenClient(EventLoop* loop, const std::vector<InetAddress>& peers)
			: loop_(loop)
	{
		for (auto& peer: peers) {
			auto client = new TcpClient(loop, peer);
			client->setConnectionCallback(std::bind(
					&NQueenClient::onConnection, this, _1));
			client->setMessageCallback(std::bind(
					&NQueenClient::onMessage, this, _1, _2));
			client->setErrorCallback(std::bind(
					&NQueenClient::onError, this));
			clients_.emplace_back(client);
		}
	}

	void start()
	{
		for (auto& client: clients_)
			client->start();
	}

	void setAllConnectedCallback(const AllConnectedCallback& cb)
	{ allConnectedCallback_ = cb; }

	void setResponseCallback(const ResponseCallback& cb)
	{ responseCallback_ = cb; }

	void onError()
	{
		forceCloseAllQuit();
	}

private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		INFO("connection %s is [%s]",
			 conn->name().c_str(),
			 conn->connected() ? "up":"down");

		if (conn->connected()) {
			connections_.push_back(conn);
			if (connections_.size() == clients_.size()) {
				if (allConnectedCallback_)
					allConnectedCallback_(connections_);
				else {
					INFO("all connected, now quit");
					onError();
				}
			}
		}
		else {
			ERROR("connection %s closed by peer", conn->name().c_str());
			onError();
		}
	}

	void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
	{
		TRACE("connection %s recv %lu bytes",
			  conn->name().c_str(),
			  buffer.readableBytes());

		while (true) {
			const char* crlf = buffer.findCRLF();
			if (crlf == nullptr)
				break;

			std::stringstream in(std::string(buffer.peek(), crlf));
			buffer.retrieveUntil(crlf + 2);

			int64_t response;
			if (in >> response && response >= 0) {
				if (responseCallback_)
					responseCallback_(conn, response);
			}
			else {
				ERROR("bad response");
				onError();
			}
		}
	}

	void forceCloseAllQuit()
	{
		for (auto& conn: connections_)
			conn->forceClose();
		loop_->queueInLoop(std::bind(&EventLoop::quit, loop_));
	}

private:
	typedef std::unique_ptr<TcpClient> TcpClientPtr;
	typedef std::vector<TcpClientPtr> TcpClientList;

	EventLoop* loop_;
	TcpClientList clients_;
	TcpConnectionList connections_;
	AllConnectedCallback allConnectedCallback_;
	ResponseCallback responseCallback_;
};

const static int64_t answerSheet[] = {0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200, 73712, 365596, 2279184, 14772512, 95815104, 666090624, 4968057848, 39029188884, 314666222712, 2691008701644, 24233937684440, 227514171973736, 2207893435808352, 22317699616364044, 234907967154122528};

class NQueenSolver: noncopyable
{
public:
	typedef NQueenClient::TcpConnectionList TcpConnectionList;

	NQueenSolver(EventLoop* loop, const std::vector<InetAddress>& peers, int nQueen)
			: loop_(loop),
			  nQueen_(nQueen),
			  client_(loop, peers),
			  answer(0),
			  totalRequest(0),
			  totalResponse(0)
	{
		assert(nQueen > 0);
		client_.setAllConnectedCallback(std::bind(
				&NQueenSolver::sendRequest, this, _1));
		client_.setResponseCallback(std::bind(
				&NQueenSolver::onResponse, this, _1, _2));
	}

	void start() { client_.start(); }

private:
	void sendRequest(const TcpConnectionList& conns)
	{
		assert(!conns.empty());
		if (nQueen_ <= 8) {
			// no concurrency
			totalRequest = 1;
			conns[0]->send(std::to_string(nQueen_) + "\r\n");
		}
		else if (nQueen_ <= 15) {
			// level 1 concurrency
			totalRequest = nQueen_;
			for (int i = 0; i < nQueen_; ++i) {
				size_t index = i % conns.size();
				auto& conn = conns[index];
				conn->send(std::to_string(nQueen_) + ' ' +
								   std::to_string(i) + "\r\n");
			}
		}
		else {
			// level 2 concurrency
			totalRequest = nQueen_ * nQueen_;
			for (int i = 0; i < nQueen_; ++i) {
				for (int j = 0; j < nQueen_; ++j) {
					size_t index = (i * nQueen_ + j) % conns.size();
					auto& conn = conns[index];
					conn->send(std::to_string(nQueen_) + ' ' +
									   std::to_string(i) + ' ' +
									   std::to_string(j) + "\r\n");
				}
			}
		}

		INFO("waiting...");
	}

	void onResponse(const TcpConnectionPtr&, int64_t count)
	{
		assert(count >= 0);
		answer += count;
		totalResponse++;
		if (totalResponse == totalRequest) {
			INFO("answer of %d queen: %ld, %s",
				 nQueen_, answer,
				 answer == answerSheet[nQueen_] ? "right":"wrong");
			client_.onError();
		}
	}

private:
	EventLoop* loop_;
	const int nQueen_;
	NQueenClient client_;

	int64_t answer;
	int totalRequest;
	int totalResponse;
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
		uint16_t port = static_cast<uint16_t>(atoi(argv[i+1]));
		peers.emplace_back(ip, port);
	}

	return peers;
}

int main(int argc, char** argv)
{
	//setLogLevel(LOG_LEVEL_TRACE);

	EventLoop loop;
	std::vector<InetAddress> peers = parseArgs(argc, argv);
	NQueenSolver client(&loop, peers, 18);
	client.start();
	loop.loop();
}