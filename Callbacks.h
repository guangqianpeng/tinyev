//
// Created by frank on 17-9-1.
//

#ifndef TINYEV_CALLBACKS_H
#define TINYEV_CALLBACKS_H

#include <memory>
#include <functional>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class Buffer;
class TcpConnection;
class InetAddress;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr&, Buffer&)> MessageCallback;
typedef std::function<void()> ErrorCallback;
typedef std::function<void(int sockfd,
						   const InetAddress& local,
						   const InetAddress& peer)> NewConnectionCallback;

typedef std::function<void()> Task;
typedef std::function<void(size_t index)> ThreadInitCallback;

void defaultThreadInitCallback(size_t index);
void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer& buffer);

#endif //TINYEV_CALLBACKS_H
