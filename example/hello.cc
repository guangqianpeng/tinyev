//
// Created by frank on 17-9-2.
//

#include "Logger.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpServer.h"

class HelloServer
{
public:
	HelloServer(EventLoop* loop, const InetAddress& addr)
			: server(loop, addr)
	{
		server.setConnectionCallback(std::bind(
				&HelloServer::onConnection, this, _1));
		server.setNumThread(1);
	}

	void start() { server.start(); }

	void onConnection(const TcpConnectionPtr& conn)
	{
		INFO("connection %s is [%s]",
			  conn->name().c_str(),
			  conn->connected() ? "up":"down");

		if (conn->connected()) {
			conn->send("[tinyev hello server]\n");
			conn->shutdown();
		}
	}

private:
	TcpServer server;
};

int main()
{
	setLogLevel(LOG_LEVEL_TRACE);
	EventLoop loop;
	InetAddress addr(9877);
	HelloServer server(&loop, addr);
	server.start();
	loop.loop();
}