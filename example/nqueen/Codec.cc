//
// Created by frank on 17-9-11.
//

#include <sstream>

#include <tinyev/TcpConnection.h>
#include <tinyev/Logger.h>

#include "Codec.h"

using namespace ev;

namespace
{

bool parseRequest(const std::string& str, Request& rqst)
{
    std::istringstream in(str);
    if (in >> rqst.nQueen) {
        uint32_t col;
        while (in >> col)
            rqst.cols.push_back(col);
        return true;
    }
    return false;
}

std::string toString(const Request& rqst)
{
    std::string str = std::to_string(rqst.nQueen);
    for (auto col: rqst.cols) {
        str.push_back(' ');
        str.append(std::to_string(col));
    }
    return str.append("\r\n");
}

bool parseResponse(const std::string& str, Response& rsps)
{
    std::istringstream in(str);
    if (in >> rsps.count) {
        uint32_t col;
        while (in >> col)
            rsps.cols.push_back(col);
        return true;
    }
    return false;
}

std::string toString(const Response& rsps)
{
    std::string str = "# ";
    str.append(std::to_string(rsps.count));
    for (auto col: rsps.cols) {
        str.push_back(' ');
        str.append(std::to_string(col));
    }
    return str.append("\r\n");
}

bool parseNCores(const std::string& str, uint32_t& nCores)
{
    std::istringstream in(str);
    if (in >> nCores)
        return true;
    return false;
}

std::string toString(uint32_t nCores)
{
    std::string str = "$ ";
    str.append(std::to_string(nCores));
    return str.append("\r\n");
}

void defaultParseErrorCallback(const TcpConnectionPtr& conn)
{
    WARN("%s parse error, forceClose", conn->name().c_str());
    conn->forceClose();
}

}

Codec::Codec(const RequestCallback& cb):
        requestCallback_(cb),
        parseErrorCallback_(defaultParseErrorCallback)
{}

Codec::Codec(const ResponseCallback& cb1, const TellCoresCallback& cb2):
        responseCallback_(cb1),
        tellCoresCallback_(cb2),
        parseErrorCallback_(defaultParseErrorCallback)
{}

void Codec::parseMessage(const TcpConnectionPtr& conn, Buffer& buffer)
{
    while (true) {
        const char *crlf = buffer.findCRLF();
        if (crlf == nullptr)
            break;
        const char *peek = buffer.peek();

        std::string str(peek, crlf);
        buffer.retrieveUntil(crlf + 2);

        if (str.empty())
            continue;

        switch (str[0]) {
            case '$':
            {
                uint32_t nCores;
                if (parseNCores(str.substr(1), nCores)) {
                    if (tellCoresCallback_)
                        tellCoresCallback_(conn, nCores);
                }
                else {
                    parseErrorCallback_(conn);
                    return;
                }
                break;
            }
            case '#':
            {
                Response rsps;
                if (parseResponse(str.substr(1), rsps)) {
                    if (responseCallback_)
                        responseCallback_(conn, rsps);
                }
                else {
                    parseErrorCallback_(conn);
                    return;
                }
                break;
            }
            case '0'...'9':
            {
                Request rqst;
                if (parseRequest(str, rqst)) {
                    if (requestCallback_)
                        requestCallback_(conn, rqst);
                }
                else {
                    parseErrorCallback_(conn);
                    return;
                }
                break;
            }
            default:
                parseErrorCallback_(conn);
                return;
        }
    }
}

void Codec::send(const TcpConnectionPtr& conn, const Request& rqst)
{ conn->send(toString(rqst)); }

void Codec::send(const TcpConnectionPtr& conn, const Response& rsps)
{ conn->send(toString(rsps)); }

void Codec::send(const TcpConnectionPtr& conn, uint32_t nCores)
{ conn->send(toString(nCores)); }