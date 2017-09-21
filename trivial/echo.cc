//
// Created by frank on 17-9-1.
//

#include "Logger.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpServer.h"

using namespace tinyev;

class EchoServer
{
public:
	EchoServer(EventLoop* loop, const InetAddress& addr)
			: server(loop, addr)
	{
		server.setConnectionCallback(std::bind(
				&EchoServer::onConnection, this, _1));
		server.setMessageCallback(std::bind(
				&EchoServer::onMessage, this, _1, _2));
	}

	void start() { server.start(); }

	void onConnection(const TcpConnectionPtr& conn)
	{
		INFO("connection %s is [%s]",
			 conn->name().c_str(),
			 conn->connected() ? "up":"down");

		if (conn->connected())
			conn->send("[tinyev echo server]\n");
	}

	void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
	{
		TRACE("connection %s recv %lu bytes",
			 conn->name().c_str(),
			 buffer.readableBytes());

		conn->send(buffer);
	}

private:
	TcpServer server;
};

int main()
{
	setLogLevel(LOG_LEVEL_TRACE);
	EventLoop loop;
	InetAddress addr(9877);
	EchoServer server(&loop, addr);
	server.start();
	loop.loop();
}