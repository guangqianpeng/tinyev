//
// Created by frank on 17-9-4.
//

#include "Logger.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpClient.h"

#include <sstream>
#include <unordered_map>
#include <string>

class NQueenClient: noncopyable
{
public:
	typedef std::unordered_map<TcpConnectionPtr, uint> TcpConnectionMap;
	typedef std::function<void(const TcpConnectionMap&)> AllConnectedCallback;
	typedef std::function<void(const TcpConnectionPtr&, int64_t, const std::vector<uint>&)> ResponseCallback;

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
	{ forceCloseAllQuit(); }

private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		INFO("connection %s is [%s]",
			 conn->name().c_str(),
			 conn->connected() ? "up":"down");

		if (conn->disconnected()) {
			WARN("connection %s closed", conn->name().c_str());
			onError();
		}
	}

	void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
	{
		TRACE("connection %s recv %lu bytes",
			  conn->name().c_str(),
			  buffer.readableBytes());

		while (true) {
			const char *crlf = buffer.findCRLF();
			if (crlf == nullptr)
				break;

			const char *peek = buffer.peek();
			char cmd = *peek;

			std::stringstream in(std::string(peek + 1, crlf));
			buffer.retrieveUntil(crlf + 2);

			if (cmd == '$') {
				assert(connections_.find(conn) == connections_.end());
				uint cores;
				if (in >> cores)
					connections_[conn] = cores;
				else {
					ERROR("bad cores");
					onError();
					break;
				}

				INFO("server %s has %u cores",
					 conn->peer().toIpPort().c_str(), cores);

				if (connections_.size() == clients_.size()) {
					allConnectedCallback_(connections_);
				}
			}
			else if (cmd == '#') {

				int64_t count;
				if (in >> count && count >= 0) {
					std::vector<uint> queens;
					uint col;
					while (in >> col)
						queens.push_back(col);
					if (responseCallback_)
						responseCallback_(conn, count, queens);
				} else {
					ERROR("bad response");
					onError();
					break;
				}
			}
		}
	}

	void forceCloseAllQuit()
	{
		for (auto& conn: connections_)
			conn.first->forceClose();
		loop_->queueInLoop(std::bind(&EventLoop::quit, loop_));
	}

private:
	typedef std::unique_ptr<TcpClient> TcpClientPtr;
	typedef std::vector<TcpClientPtr> TcpClientList;

	EventLoop* loop_;
	TcpClientList clients_;
	TcpConnectionMap connections_;
	AllConnectedCallback allConnectedCallback_;
	ResponseCallback responseCallback_;
};

const static int64_t answerSheet[] = {0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200, 73712, 365596, 2279184, 14772512, 95815104, 666090624, 4968057848, 39029188884, 314666222712, 2691008701644, 24233937684440, 227514171973736, 2207893435808352, 22317699616364044, 234907967154122528};

class NQueenSolver: noncopyable
{
public:
	typedef NQueenClient::TcpConnectionMap TcpConnectionMap;

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
				&NQueenSolver::sendRequests, this, _1));
		client_.setResponseCallback(std::bind(
				&NQueenSolver::onResponse, this, _1, _2, _3));
		initRequests();
	}

	void start() { client_.start(); }

private:
	void sendRequests(const TcpConnectionMap &conns)
	{
		assert(!conns.empty());
		assert(totalRequest > 0);
		assert(totalResponse == 0);

		for (auto& p: conns) {
			auto& conn = p.first;
			uint cores = p.second;
			assert(cores > 0);

			for (uint i = 0; i < cores; ++i) {
				if (!requests_.empty()) {
					sendOneRequest(conn);
				}
				else break;
			}
			if (requests_.empty())
				break;
		}

		INFO("waiting...");
	}

	void onResponse(const TcpConnectionPtr& conn, int64_t count, const std::vector<uint>& queens)
	{
		assert(count >= 0);

		if (!queens.empty()) {
			// middle col
			if (nQueen_ % 2 == 1 &&
				queens[0] + 1 == (static_cast<uint>(nQueen_) + 1) / 2)
				answer += count;
			else
				answer += 2 * count;
		}
		else
			answer += count;

		totalResponse++;
		if (totalResponse % 10 == 0) {
			INFO("[%d/%d] sub problems solved, %ld answers found",
				 totalResponse, totalRequest, answer);
		}
		if (totalResponse == totalRequest) {
			INFO("answer of %d queen: %ld, %s",
				 nQueen_, answer,
				 answer == answerSheet[nQueen_] ? "right":"wrong");
			client_.onError();
		}

		if (!requests_.empty())
			sendOneRequest(conn);
	}

	void initRequests()
	{

		for (int i = 0; i < (nQueen_ + 1) / 2; ++i) {
			for (int j = 0; j < nQueen_; ++j) {
				requests_.push_back(std::to_string(nQueen_) + ' ' +
									std::to_string(i) + ' ' +
									std::to_string(j) + "\r\n");
				totalRequest++;
			}
		}
	}


	void sendOneRequest(const TcpConnectionPtr& conn)
	{
		assert(!requests_.empty());
		conn->send(requests_.back());
		requests_.pop_back();
	}

private:
	EventLoop* loop_;
	const int nQueen_;
	NQueenClient client_;
	std::vector<std::string> requests_;

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
	NQueenSolver client(&loop, peers, 20);
	client.start();
	loop.loop();
}