//
// Created by frank on 17-9-11.
//

#include <sstream>

#include "Logger.h"
#include "TcpConnection.h"
#include "Codec.h"

#define COMMAND_SUM 	'!'
#define COMMAND_LESS 	'@'
#define ANSWER_SUM		'#'
#define ANSWER_LESS		'$'

namespace
{

std::string toString(__int128 x)
{
	static const char digits[19] =
			{ '9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
			  '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	static const char* zero = &digits[9];

	std::string str;
	do {
		// lsd - least significant digit
		auto lsd = static_cast<int>(x % 10);
		x /= 10;
		str.push_back(zero[lsd]);
	} while (x != 0);

	if (x < 0)
		str.push_back('-');

	std::reverse(str.begin(), str.end());
	return str;
}

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
	conn->send("bad message\r\n");
	conn->forceClose();
}

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
			case COMMAND_SUM:
				cmdSumCallback_(conn);
				break;
			case COMMAND_LESS:
				if (!parseCommandLess(conn, str.substr(1)))
					return;
				break;
			case ANSWER_SUM:
				if (!parseAnswerSum(conn, str.substr(1)))
					return;
				break;
			case ANSWER_LESS:
				if (!parseAnswerLess(conn, str.substr(1)))
					return;
				break;
			default:
				parseErrorCallback_(conn);
				return;
		}
	}
}

void Codec::sendCommandSum(const TcpConnectionPtr &conn)
{
	std::string str(1, COMMAND_SUM);
	conn->send(str.append("\r\n"));
}

void Codec::sendCommandLess(const TcpConnectionPtr &conn, int64_t target)
{
	std::string str(1, COMMAND_LESS);
	str.append(std::to_string(target));
	conn->send(str.append("\r\n"));
}

void Codec::sendAnswerSum(const TcpConnectionPtr &conn, __int128 sum, int64_t count)
{
	std::string str(1, ANSWER_SUM);
	str.append(toString(sum));
	str.push_back(' ');
	str.append(std::to_string(count));
	conn->send(str.append("\r\n"));
}

void Codec::sendAnswerLess(const TcpConnectionPtr &conn, int64_t lessCount, int64_t equalCount)
{
	std::string str(1, ANSWER_LESS);
	str.append(std::to_string(lessCount));
	str.push_back(' ');
	str.append(std::to_string(equalCount));
	conn->send(str.append("\r\n"));
}

bool Codec::parseCommandLess(const TcpConnectionPtr& conn, const std::string& arg)
{
	std::istringstream in(arg);
	int64_t target;
	if (in >> target && cmdLessCallback_) {
		cmdLessCallback_(conn, target);
		return true;
	}
	parseErrorCallback_(conn);
	return false;
}

bool Codec::parseAnswerSum(const TcpConnectionPtr& conn, const std::string& arg)
{
	__int128 sum;
	int64_t count;

	size_t len = parseInt128(arg, sum);
	if (len == 0) {
		parseErrorCallback_(conn);
		return false;
	}

	std::istringstream in(arg.substr(len));
	if (in >> count && ansSumCallback_) {
		ansSumCallback_(conn, sum, count);
		return true;
	}
	parseErrorCallback_(conn);
	return false;
}

bool Codec::parseAnswerLess(const TcpConnectionPtr& conn, const std::string& arg)
{
	std::istringstream in(arg);
	int64_t lessCount;
	int64_t equalCount;
	if (in >> lessCount >> equalCount && ansLessCallback_) {
		ansLessCallback_(conn, lessCount, equalCount);
		return true;
	}
	parseErrorCallback_(conn);
	return false;
}