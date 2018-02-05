//
// Created by frank on 17-9-11.
//

#include <sstream>

#include <tinyev/Logger.h>
#include <tinyev/TcpConnection.h>

#include "Codec.h"

#define QUERY 	'@'
#define STATE	'#'
#define ANSWER	'$'

using namespace ev;

namespace
{

size_t parseInt128(const std::string& arg, __int128& ret)
{
	if (arg.empty())
		return 0;

	size_t i = (arg[0] == '-' ? 1 : 0);

	if (i == arg.size() || !isdigit(arg[i]))
		return 0;

	ret = 0;
	for (; isdigit(arg[i]); i++) {
		ret *= 10;
		ret += arg[i] - '0';
	}

	if (arg[0] == '-')
		ret = -ret;

	return i;
}

void defaultParseErrorCallback(const TcpConnectionPtr& conn)
{
	WARN("%s parse error, forceClose", conn->name().c_str());
	conn->send("bad message\r\n"sv);
	conn->forceClose();
}

}

std::string toString(__int128 x)
{
	static const char digits[19] =
			{ '9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
			  '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	static const char* zero = &digits[9];

	bool negtive = (x < 0);
	std::string str;
	do {
		// lsd - least significant digit
		auto lsd = static_cast<int>(x % 10);
		x /= 10;
		str.push_back(zero[lsd]);
	} while (x != 0);

	if (negtive)
		str.push_back('-');

	std::reverse(str.begin(), str.end());
	return str;
}

Codec::Codec()
		: parseErrorCallback_(defaultParseErrorCallback)
{}

void Codec::parseMessage(const TcpConnectionPtr &conn, Buffer &buffer)
{
	while (true) {
		const char *crlf = buffer.findCRLF();
		if (crlf == nullptr)
			break;

		std::string str(buffer.peek(), crlf);
		buffer.retrieveUntil(crlf + 2);

		if (str.empty())
			continue;

		switch (str[0]) {
			case QUERY:
				if (!parseQuery(conn, str.substr(1)))
					return;
				break;
			case STATE:
				if (!parseState(conn, str.substr(1)))
					return;
				break;
			case ANSWER:
				if (!parseAnswer(conn, str.substr(1)))
					return;
				break;
			default:
				parseErrorCallback_(conn);
				return;
		}
	}
}

void Codec::sendQuery(const TcpConnectionPtr &conn, int64_t guess)
{
	std::string str(1, QUERY);
	str.append(std::to_string(guess));
	conn->send(str.append("\r\n"));
}

void Codec::sendState(const TcpConnectionPtr &conn,
					  __int128 sum, int64_t count, int64_t min, int64_t max)
{
	std::string str(1, STATE);
	str.append(toString(sum));
	str.push_back(' ');
	str.append(std::to_string(count));
	str.push_back(' ');
	str.append(std::to_string(min));
	str.push_back(' ');
	str.append(std::to_string(max));
	conn->send(str.append("\r\n"));
}

void Codec::sendAnswer(const TcpConnectionPtr &conn,
					   int64_t lessCount, int64_t equalCount)
{
	std::string str(1, ANSWER);
	str.append(std::to_string(lessCount));
	str.push_back(' ');
	str.append(std::to_string(equalCount));
	conn->send(str.append("\r\n"));
}

bool Codec::parseQuery(const TcpConnectionPtr &conn, const std::string &arg)
{
	std::istringstream in(arg);
	int64_t target;
	if (in >> target && queryCallback_) {
		queryCallback_(conn, target);
		return true;
	}
	parseErrorCallback_(conn);
	return false;
}

bool Codec::parseState(const TcpConnectionPtr &conn, const std::string &arg)
{
	__int128 sum;
	int64_t count;
	int64_t min, max;

	size_t len = parseInt128(arg, sum);
	if (len == 0) {
		parseErrorCallback_(conn);
		return false;
	}

	std::istringstream in(arg.substr(len));
	if (in >> count >> min >> max) {
		tellStateCallback_(conn, sum, count, min, max);
		return true;
	}
	parseErrorCallback_(conn);
	return false;
}

bool Codec::parseAnswer(const TcpConnectionPtr &conn, const std::string &arg)
{
	std::istringstream in(arg);
	int64_t lessCount;
	int64_t equalCount;
	if (in >> lessCount >> equalCount) {
		if (answerCallback_) {
			answerCallback_(conn, lessCount, equalCount);
			return true;
		}
	}
	parseErrorCallback_(conn);
	return false;
}