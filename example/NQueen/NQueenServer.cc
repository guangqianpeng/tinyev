//
// Created by frank on 17-9-3.
//

#include <cstdint>
#include <cassert>
#include <atomic>
#include <sstream>

#include "Logger.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "ThreadPool.h"

class BackTrack
{
public:
	static int64_t solve(uint nQueen, const std::vector<uint>& queens)
	{
		BackTrack b(nQueen);
		if (b.tryPutQueens(queens)) {
			uint startRow = static_cast<uint>(queens.size());
			b.search(startRow);
		}
		return b.count;
	}

	const static uint kMaxQueens = 32;

private:
	const uint N;
	int64_t count;
	// bitmask, 0 is available
	uint32_t col[kMaxQueens];
	uint32_t diag[kMaxQueens];
	uint32_t antidiag[kMaxQueens];

	explicit
	BackTrack(uint nQueens)
			: N(nQueens),
			  count(0),
			  col{0},
			  diag{0},
			  antidiag{0}
	{
		assert(nQueens <= kMaxQueens && nQueens > 0);
	}

	bool tryPutQueens(const std::vector<uint> &queens)
	{
		assert(queens.size() <= N);

		for (size_t row = 0; row < queens.size(); ++row) {

			assert(queens[row] < N);

			// bitmask, 0 is available
			uint32_t unavail = col[row] | diag[row] | antidiag[row];
			uint32_t mask = (1u << queens[row]);

			if (unavail & mask)
				return false;

			if (row == N - 1) {
				count = 1;
				return false;
			}

			col[row + 1] = (col[row] | mask);
			diag[row + 1] = ((diag[row] | mask) >> 1);
			antidiag[row + 1] = ((antidiag[row] | mask) << 1);
		}
		return true;
	}

	void search(uint row)
	{
		uint32_t avail = col[row] | diag[row] | antidiag[row];
		avail = ~avail; // bitmask, 1 is available

		while (avail) {

			uint i = __builtin_ctz(avail);
			if (i >= N) break;

			if (row == N - 1)
				count++;
			else {
				uint32_t mask = (1u << i);
				col[row + 1] = (col[row] | mask);
				diag[row + 1] = ((diag[row] | mask) >> 1);
				antidiag[row + 1] = ((antidiag[row] | mask) << 1);
				search(row + 1);
			}
			avail &= avail - 1;
		}
	}
};

class NQueenServer
{
public:
	NQueenServer(EventLoop* loop, const InetAddress& addr)
			  : server(loop, addr),
				threadPool_(4)
	{
		server.setConnectionCallback(std::bind(
				&NQueenServer::onConnection, this, _1));
		server.setMessageCallback(std::bind(
				&NQueenServer::onMessage, this, _1, _2));
	}

	void start() { server.start(); }

private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		INFO("connection %s is [%s]",
			 conn->name().c_str(),
			 conn->connected() ? "up":"down");
	}

	void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
	{
		TRACE("connection %s recv %lu bytes",
			  conn->name().c_str(),
			  buffer.readableBytes());

#define Error(str) do { conn->send(str"\r\n"); return; } while(false)

		while (true) {
			const char *crlf = buffer.findCRLF();
			if (crlf == nullptr)
				break;
			const char *peek = buffer.peek();

			std::stringstream in(std::string(peek, crlf));

			buffer.retrieveUntil(crlf + 2);

			uint nQueens;
			if (!(in >> nQueens))
				Error("invalid queen number");

			if (nQueens == 0 || nQueens > BackTrack::kMaxQueens)
				Error("invalid queen number");

			std::vector<uint> queens;
			uint q;
			while (in >> q) {
				if (q >= nQueens)
					Error("invalid queen position");
				queens.push_back(q);
			}

			in.clear();
			std::string remain;
			if (in >> remain)
				Error("bad request");

			if (queens.size() > nQueens)
				Error("too many queens");

			threadPool_.runTask([nQueens, queens, conn](){
				int64_t count = BackTrack::solve(nQueens, queens);
				conn->send(std::to_string(count) + "\r\n");
			});
		}

#undef Error
	}

private:
	TcpServer server;
	ThreadPool threadPool_;
};

int main()
{
	setLogLevel(LOG_LEVEL_TRACE);
	EventLoop loop;
	InetAddress addr(9877);
	NQueenServer server(&loop, addr);
	server.start();
	loop.loop();
}
