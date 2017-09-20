//
// Created by frank on 17-9-11.
//

#ifndef TINYEV_CODEC_H
#define TINYEV_CODEC_H

#include <cstdint>
#include <vector>
#include <functional>

#include "Callbacks.h"
#include "noncopyable.h"

struct Request
{
	uint32_t nQueen;
	std::vector<uint32_t> cols;
};

struct Response
{
	int64_t count;
	std::vector<uint32_t> cols;
};

class Codec: tinyev::noncopyable
{
public:
	typedef std::function<void(const tinyev::TcpConnectionPtr&, const Request&)> RequestCallback;
	typedef std::function<void(const tinyev::TcpConnectionPtr&, const Response&)> ResponseCallback;
	typedef std::function<void(const tinyev::TcpConnectionPtr&, uint32_t)> TellCoresCallback;
	typedef std::function<void(const tinyev::TcpConnectionPtr&)> ParseErrorCallback;

	explicit
	Codec(const RequestCallback& cb);
	Codec(const ResponseCallback& cb1, const TellCoresCallback& cb2);

	void setParseErrorCallback(const ParseErrorCallback& cb)
	{ parseErrorCallback_ = cb; }

	void parseMessage(const tinyev::TcpConnectionPtr& conn, tinyev::Buffer& buffer);

	void send(const tinyev::TcpConnectionPtr& conn, const Request& rqst);
	void send(const tinyev::TcpConnectionPtr& conn, const Response& rsps);
	void send(const tinyev::TcpConnectionPtr& conn, uint32_t nCores);

private:
	RequestCallback requestCallback_;
	ResponseCallback  responseCallback_;
	TellCoresCallback tellCoresCallback_;
	ParseErrorCallback parseErrorCallback_;
};


#endif //TINYEV_CODEC_H
