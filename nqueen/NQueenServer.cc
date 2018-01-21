//
// Created by frank on 17-9-3.
//

#include <cstdint>
#include <cassert>

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/TcpServer.h>
#include <tinyev/ThreadPool.h>

#include "Codec.h"

using namespace ev;

class BackTrack
{
public:
    static int64_t solve(uint32_t nQueen, const std::vector<uint32_t>& queens)
    {
        BackTrack b(nQueen);
        if (b.tryPutQueens(queens)) {
            auto startRow = static_cast<uint32_t>(queens.size());
            b.search(startRow);
        }
        return b.count;
    }

    const static uint kMaxQueens = 32;

private:
    const uint32_t N;
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

    bool tryPutQueens(const std::vector<uint32_t> &queens)
    {
        if (queens.size() > N)
            return false;

        for (size_t row = 0; row < queens.size(); ++row) {

            if (queens[row] >= N)
                return false;

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
    NQueenServer(EventLoop* loop, const InetAddress& addr, size_t threadPoolSize)
              : loop_(loop),
                codec_(std::bind(&NQueenServer::onRequest, this, _1, _2)),
                server_(loop, addr),
                threadPool_(threadPoolSize)
    {
        server_.setConnectionCallback(std::bind(
                &NQueenServer::onConnection, this, _1));
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
        if (conn->connected()) {
            auto nCores = static_cast<uint32_t>(threadPool_.numThreads());
            codec_.send(conn, nCores);
        }
    }

    void onRequest(const TcpConnectionPtr& conn, const Request& rqst)
    {
        TRACE("connection %s recv one request",
              conn->name().c_str());

        threadPool_.runTask([this, rqst, conn](){
            Response rsps;
            rsps.count = BackTrack::solve(rqst.nQueen, rqst.cols);
            rsps.cols = rqst.cols;
            codec_.send(conn, rsps);
        });
    }

private:
    EventLoop* loop_;
    Codec codec_;
    TcpServer server_;
    ThreadPool threadPool_;
};

int main(int argc, char** argv)
{
    setLogLevel(LOG_LEVEL_TRACE);

    size_t threadPoolSize = 0;

    if (argc == 2) {
        int n = atoi(argv[1]);
        if (n > 0)
            threadPoolSize = static_cast<size_t>(n);
    }

    if (threadPoolSize == 0)
        threadPoolSize = std::thread::hardware_concurrency();

#ifndef NDEBUG
    INFO("debug mode");
#endif

    EventLoop loop;
    InetAddress addr(9877);
    NQueenServer server(&loop, addr, threadPoolSize);
    server.start();
    loop.loop();
}
