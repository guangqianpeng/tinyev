//
// Created by frank on 17-9-4.
//

#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpClient.h"

TcpClient::TcpClient(EventLoop* loop, const InetAddress& peer)
		: loop_(loop),
		  connector_(loop, peer),
		  connectionCallback_(defaultConnectionCallback),
		  messageCallback_(defaultMessageCallback)
{
	connector_.setNewConnectionCallback(std::bind(
			&TcpClient::newConnection, this, _1, _2, _3));
}

TcpClient::~TcpClient()
{
	if (connection_ && !connection_->disconnected())
		connection_->forceClose();
}

void TcpClient::newConnection(int connfd, const InetAddress& local, const InetAddress& peer)
{
	loop_->assertInLoopThread();
	auto conn = std::make_shared<TcpConnection>
			(loop_, connfd, local, peer);
	connection_ = conn;
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallBack(std::bind(
			&TcpClient::closeConnection, this, _1));
	// enable and tie channel
	conn->connectEstablished();
	connectionCallback_(conn);
}

void TcpClient::closeConnection(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	assert(connection_ != nullptr);
	connection_.reset();
	connectionCallback_(conn);
}