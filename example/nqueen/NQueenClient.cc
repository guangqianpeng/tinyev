//
// Created by frank on 17-9-4.
//

#include <unordered_set>

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/TcpClient.h>

#include "Codec.h"

using namespace ev;

const static int64_t answerSheet[] = {0, 1, 0, 0, 2, 10, 4, 40, 92, // 8
                                      352, 724, 2680, 14200, 73712, // 13
                                      365596, 2279184, 14772512, 95815104, // 17
                                      666090624, 4968057848, 39029188884,  // 20
                                      314666222712, 2691008701644, // 22
                                      24233937684440, 227514171973736, // 24
                                      2207893435808352, 22317699616364044, // 26
                                      234907967154122528}; // 27

class NQueenClient: noncopyable
{
public:
    NQueenClient(EventLoop* loop, const std::vector<InetAddress>& peers, uint32_t nQueen)
            : loop_(loop),
              nQueen_(nQueen),
              codec_(std::bind(&NQueenClient::onMessage, this, _1, _2),
                     std::bind(&NQueenClient::onTellCores, this, _1, _2)),
              totalRequests_(0),
              totalResponse_(0),
              answers_(0)
    {
        for (auto& peer: peers) {
            auto client = new TcpClient(loop, peer);
            client->setConnectionCallback(std::bind(
                    &NQueenClient::onConnection, this, _1));
            client->setMessageCallback(std::bind(
                    &Codec::parseMessage, &codec_, _1, _2));
            client->setErrorCallback(std::bind(
                    &NQueenClient::onConnectError, this));
            clients_.emplace_back(client);
        }
        initRequests();
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
        if (conn->connected())
            connections_.insert(conn);
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

    void onMessage(const TcpConnectionPtr &conn, const Response &rsps)
    {
        assert(rsps.count >= 0);

        if (!rsps.cols.empty()) {
            // middle col
            if (nQueen_ % 2 == 1 &&
                rsps.cols[0] + 1 == (static_cast<uint>(nQueen_) + 1) / 2)
                answers_ += rsps.count;
            else
                answers_ += 2 * rsps.count;
        }
        else
            answers_ += rsps.count;

        totalResponse_++;
        if (totalResponse_ % 10 == 0) {
            INFO("[%d/%d] sub problems solved, %ld answers found",
                 totalResponse_, totalRequests_, answers_);
        }
        if (totalResponse_ == totalRequests_) {
            INFO("answer of %d queen: %ld, %s",
                 nQueen_, answers_,
                 answers_ == answerSheet[nQueen_] ? "right":"wrong");
            conn->shutdown();
        }

        if (!requests_.empty())
            sendOneRequest(conn);
    }

    void onTellCores(const TcpConnectionPtr& conn, uint32_t nCores)
    {
        for (uint32_t i = 0; i < nCores; ++i) {
            if (!requests_.empty()) {
                sendOneRequest(conn);
            }
            else break;
        }
    }

    void initRequests()
    {
        Request rqst;
        rqst.nQueen = nQueen_;
        if (nQueen_ == 1) {
            requests_.push_back(rqst);
            totalRequests_ = 1;
        }
        else {
            rqst.cols.resize(2);
            for (uint32_t i = 0; i < (nQueen_ + 1) / 2; ++i) {
                for (uint32_t j = 0; j < nQueen_; ++j) {
                    rqst.cols[0] = i;
                    rqst.cols[1] = j;
                    requests_.push_back(rqst);
                    totalRequests_++;
                }
            }
        }
    }


    void sendOneRequest(const TcpConnectionPtr& conn)
    {
        assert(!requests_.empty());
        codec_.send(conn, requests_.back());
        requests_.pop_back();
    }

private:
    typedef std::unique_ptr<TcpClient> TcpClientPtr;
    typedef std::vector<TcpClientPtr> TcpClientList;
    typedef std::unordered_set<TcpConnectionPtr> ConnectionSet;

    EventLoop* loop_;
    const uint32_t nQueen_;
    Codec codec_;
    std::vector<Request> requests_;
    uint32_t totalRequests_;
    uint32_t totalResponse_;
    int64_t answers_;
    TcpClientList clients_;
    ConnectionSet connections_;
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
    NQueenClient client(&loop, peers, 17);
    client.start();
    loop.loop();
}