//
// Created by frank on 17-9-11.
//

#ifndef TINYEV_CODEC_H
#define TINYEV_CODEC_H


#include <functional>

#include "Callbacks.h"
#include "noncopyable.h"

class Codec: noncopyable
{
public:
	typedef std::function<void(const TcpConnectionPtr&)> CommandSumCallback;
	typedef std::function<void(const TcpConnectionPtr&, int64_t)> CommandLessCallback;
	typedef std::function<void(const TcpConnectionPtr&, __int128, int64_t)> AnswerSumCallback;
	typedef std::function<void(const TcpConnectionPtr&, int64_t, int64_t)> AnswerLessCallback;
	typedef std::function<void(const TcpConnectionPtr&)> ParseErrorCallback;

	Codec();

	void parseMessage(const TcpConnectionPtr& conn, Buffer& buffer);

	void setCommandSumCallback(const CommandSumCallback& cb)
	{ cmdSumCallback_ = cb; }
	void setCommandLessCallback(const CommandLessCallback& cb)
	{ cmdLessCallback_ = cb; }
	void setAnswerSumCallback(const AnswerSumCallback& cb)
	{ ansSumCallback_ = cb; }
	void setAnswerLessCallback(const AnswerLessCallback& cb)
	{ ansLessCallback_ = cb; }

	void sendCommandSum(const TcpConnectionPtr &conn);
	void sendCommandLess(const TcpConnectionPtr &conn, int64_t target);
	void sendAnswerSum(const TcpConnectionPtr &conn, __int128 sum, int64_t count);
	void sendAnswerLess(const TcpConnectionPtr &conn, int64_t lessCount, int64_t equalCount);

private:
	bool parseCommandLess(const TcpConnectionPtr& conn, const std::string& arg);
	bool parseAnswerSum(const TcpConnectionPtr& conn, const std::string& arg);
	bool parseAnswerLess(const TcpConnectionPtr& conn, const std::string& arg);

	CommandSumCallback cmdSumCallback_;
	CommandLessCallback cmdLessCallback_;
	AnswerSumCallback ansSumCallback_;
	AnswerLessCallback ansLessCallback_;
	ParseErrorCallback parseErrorCallback_;
};


#endif //TINYEV_CODEC_H
