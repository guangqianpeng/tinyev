//
// Created by frank on 17-9-1.
//

#ifndef TINYEV_TCPCONNECTION_H
#define TINYEV_TCPCONNECTION_H

#include "noncopyable.h"
#include "Buffer.h"
#include "Callbacks.h"
#include "Channel.h"
#include "InetAddress.h"

namespace tinyev
{

class EventLoop;

class TcpConnection: noncopyable,
					 public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* loop, int sockfd,
				  const InetAddress& local,
				  const InetAddress& peer);
	~TcpConnection();

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }

	// internal use
	void setCloseCallBack(const CloseCallback& cb)
	{ closeCallback_ = cb; }

	// TcpServerSingle
	void connectEstablished();

	bool connected() const;
	bool disconnected() const;

	const InetAddress& local() const
	{ return local_; }
	const InetAddress& peer() const
	{ return peer_; }

	std::string name() const
	{ return peer_.toIpPort() + " -> " + local_.toIpPort(); }

	// I/O operations are thread safe
	void send(const char* data, size_t len);
	void send(const std::string& message);
	void send(Buffer& buffer);
	void shutdown();
	void forceClose();

private:
	void handleRead();
	void handleWrite();
	void handleClose();
	void handleError();

	void sendInLoop(const char* data, size_t len);
	void sendInLoop(const std::string& message);
	void shutdownInLoop();
	void forceCloseInLoop();

	int stateAtomicGetAndSet(int newState);

	EventLoop* loop_;
	const int sockfd_;
	Channel channel_;
	int state_;
	InetAddress local_;
	InetAddress peer_;
	Buffer inputBuffer_;
	Buffer outputBuffer_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	CloseCallback closeCallback_;
};

}

#endif //TINYEV_TCPCONNECTION_H
