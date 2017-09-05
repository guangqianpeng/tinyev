//
// Created by frank on 17-9-4.
//

#ifndef TINYEV_CONNECTOR_H
#define TINYEV_CONNECTOR_H

#include <functional>

#include "InetAddress.h"
#include "Channel.h"
#include "noncopyable.h"

class EventLoop;
class InetAddress;

class Connector: noncopyable
{
public:
	Connector(EventLoop* loop, const InetAddress& peer);
	~Connector();

	void start();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{ newConnectionCallback_ = cb; }

	void setErrorCallback(const ErrorCallback& cb)
	{ errorCallback_ = cb; }

private:
	void handleWrtie();

	EventLoop* loop_;
	InetAddress peer_;
	int sockfd_;
	bool started_;
	Channel channel_;
	NewConnectionCallback newConnectionCallback_;
	ErrorCallback errorCallback_;
};


#endif //TINYEV_CONNECTOR_H
