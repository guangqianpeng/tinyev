//
// Created by frank on 17-9-11.
//

#ifndef TINYEV_CODEC_H
#define TINYEV_CODEC_H


#include <functional>

#include <tinyev/Callbacks.h>
#include <tinyev/noncopyable.h>

class Codec: ev::noncopyable
{
public:
	typedef ev::TcpConnectionPtr TcpConnectionPtr;
	typedef ev::Buffer Buffer;

	typedef std::function<void(const TcpConnectionPtr&, int64_t)> QueryCallback;
	typedef std::function<void(const TcpConnectionPtr&, int64_t, int64_t)> AnswerCallback;
	typedef std::function<void(const TcpConnectionPtr&,
							   __int128, int64_t, int64_t, int64_t)> TellStateCallback;
	typedef std::function<void(const TcpConnectionPtr&)> ParseErrorCallback;

public:
	Codec();

	void parseMessage(const TcpConnectionPtr& conn, Buffer& buffer);

	void setQueryCallback(const QueryCallback &cb)
	{ queryCallback_ = cb; }
	void setAnswerCallback(const AnswerCallback &cb)
	{ answerCallback_ = cb; }
	void setTellStateCallback(const TellStateCallback& cb)
	{ tellStateCallback_ = cb; }

	void sendQuery(const TcpConnectionPtr &conn, int64_t guess);
	void sendAnswer(const TcpConnectionPtr &conn,
					int64_t lessCount, int64_t equalCount);
	void sendState(const TcpConnectionPtr &conn,
				   __int128 sum, int64_t count, int64_t min, int64_t max);

private:
	bool parseQuery(const TcpConnectionPtr &conn, const std::string &arg);
	bool parseAnswer(const TcpConnectionPtr &conn, const std::string &arg);
	bool parseState(const TcpConnectionPtr &conn, const std::string &arg);

	QueryCallback queryCallback_;
	AnswerCallback answerCallback_;
	TellStateCallback tellStateCallback_;
	ParseErrorCallback parseErrorCallback_;
};

std::string toString(__int128 x);

#endif //TINYEV_CODEC_H
