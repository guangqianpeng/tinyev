# tinyev: A multithreaded C++ network library

[![Build Status](https://travis-ci.org/guangqianpeng/tinyev.svg?branch=master)](https://travis-ci.org/guangqianpeng/tinyev)

## 简介

tinyev是仿照muduo[1]实现的一个基于Reactor模式的多线程C++网络库，经过适当简化以后代码量约为2000行。简化的部分如下：

- 多线程依赖于C++11提供的std::thread库，而不是重新封装POSIX thread API。

- 定时器依赖于C++11提供的std::chrono库，而不是自己实现Timstamp类，也不用直接调用`gettimeofday()`。这样写的好处之一是我们不必再为定时器API的时间单位操心[2]：

  ```c++
  using namespace std::literals::chrono_literals;
  loop.runEvery(10s, [](){INFO("run every 10 seconds");});
  loop.runAfter(24h, [](){INFO("run after 24 hours");});
  loop.runAt(Clock::nowAfter(15min), [](){INFO("run 15 minutes later");});
  ```

- 默认为accept socket开启SO_RESUEPORT选项，这样每个线程都有自己的Acceptor，就不必在线程间传递connection socket。开启该选项后内核会将TCP连接分摊给各个线程，因此不必担心负载均衡的问题。

- 仅具有简单的日志输出功能，用于调试。

- 仅使用epoll，不使用poll和select。

## 示例

一个简单的echo服务器如下：

```C++
class EchoServer: noncopyable
{
public:
  EchoServer(EventLoop* loop, const InetAddress& addr)
    : loop_(loop),
      server_(loop, addr),
  {
    // set echo callback
    server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2));
  }
  void start() { server_.start(); }
  void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
  {
    // echo message
    conn->send(buffer);
  }
private:
  EventLoop* loop_;
  TcpServer server_;
};
```

这个实现非常简单，读者只需关注`onMessage`回调函数，它将收到消息发回客户端。然而，该实现有一个问题：若客户端只发送而不接收数据（即只调用`write`而不调用`read`），则TCP的流量控制（flow control）会导致数据堆积在服务端，最终会耗尽服务端内存。为解决该问题我们引入高/低水位回调：

```c++
class EchoServer: noncopyable
{
public:
  ...
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
      conn->setHighWaterMarkCallback(
            std::bind(&EchoServer::onHighWaterMark, this, _1, _2), 1024);
  }
  void onHighWaterMark(const TcpConnectionPtr& conn, size_t mark)
  {
    INFO("high water mark %lu bytes, stop read", mark);
    conn->stopRead();
  }
  void onWriteComplete(const TcpConnectionPtr& conn)
  {
    if (!conn->isReading()) {
      INFO("write complete, start read");
      conn->startRead();
    }
  }
  ...
};
```

我们新增了3个回调：`onConnection`，`onHighWaterMark`和`onWriteComplete`。当TCP连接建立时`onConnection`会设置高水位回调值（high water mark）；当send buffer达到该值时，`onHighWaterMark`会停止读socket；当send buffer全部写入内核时，`onWriteComplete`会重新开始读socket。

除此以外，我们还需要给服务器加上定时功能以清除空闲连接。实现思路是让服务器保存一个TCP连接的`std::map`，每隔几秒扫描一遍所有连接并清除超时的连接，代码在[这里](./example/echo.cc)。

然后，我们给服务器加上多线程功能。实现起来非常简单，只需加一行代码即可：

```c++
EchoServer：：void start()
{
  // set thread num here
  server_.setNumThread(2);
  server_.start();
}
```

最后，main函数如下：

```c++
int main()
{
  EventLoop loop;
  // listen address localhost:9877
  InetAddress addr(9877);
  // echo server with 4 threads and timeout of 10 seconds
  EchoServer server(&loop, addr, 4, 10s);
  // loop all other threads except this one
  server.start();
  // quit after 1 minute
  loop.runAfter(1min, [&](){ loop.quit(); });
  // loop main thread
  loop.loop();
}
```

## 安装

```shell
$ git clone git@github.com:guangqianpeng/tinyev.git
$ cd tinyev
$ git submodule update --init --recursive
$ ./build.sh 
$ ./build.sh install
```

tinyev安装在 `../tinyev-build/Release/{include, lib}`

## 更多

网络库的具体实现方法参考我的[博客](http://www.penggq.org/2017/09/%E5%86%99%E4%B8%80%E4%B8%AAC-%E5%A4%9A%E7%BA%BF%E7%A8%8B%E7%BD%91%E7%BB%9C%E5%BA%93)。

## 参考

[[1]](https://github.com/chenshuo/muduo) Muduo is a multithreaded C++ network library based on the reactor pattern.

[[2]](https://www.youtube.com/watch?v=fX2W3nNjJIo&list=PLHTh1InhhwT6bwIpRk0ZbCA0N2p1taxd6) CppCon 2017: Bjarne Stroustrup “Learning and Teaching Modern C++”. Make interfaces precisely and strongly typed.