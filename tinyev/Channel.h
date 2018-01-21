//
// Created by frank on 17-8-31.
//

#ifndef TINYEV_CHANNEL_H
#define TINYEV_CHANNEL_H

#include <functional>
#include <memory>
#include <sys/epoll.h>

#include <tinyev/noncopyable.h>

namespace ev
{

class EventLoop;

class Channel: noncopyable
{
public:
    typedef std::function<void()> ReadCallback;
    typedef std::function<void()> WriteCallback;
    typedef std::function<void()> CloseCallback;
    typedef std::function<void()> ErrorCallback;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void setReadCallback(const ReadCallback& cb)
    { readCallback_ = cb; }
    void setWriteCallback(const WriteCallback& cb)
    { writeCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }
    void setErrorCallback(const ErrorCallback& cb)
    { errorCallback_ = cb; }

    void handleEvents();

    bool polling;
    int fd() const
    { return fd_; }
    bool isNoneEvents() const
    { return events_ == 0; }
    unsigned events() const
    { return events_; }
    void setRevents(unsigned revents)
    { revents_ = revents; }

    void tie(const std::shared_ptr<void>& obj);

    void enableRead()
    { events_ |= (EPOLLIN | EPOLLPRI); update();}
    void enableWrite()
    { events_ |= EPOLLOUT; update();}
    void disableRead()
    { events_ &= ~EPOLLIN; update(); }
    void disableWrite()
    { events_ &= ~EPOLLOUT; update();}
    void disableAll()
    { events_ = 0; update();}

    bool isReading() const { return events_ & EPOLLIN; }
    bool isWriting() const { return events_ & EPOLLOUT; }

private:
    void update();
    void remove();

    void handleEventsWithGuard();

    EventLoop* loop_;
    int fd_;

    std::weak_ptr<void> tie_;
    bool tied_;

    unsigned events_;
    unsigned revents_;

    bool handlingEvents_;

    ReadCallback readCallback_;
    WriteCallback writeCallback_;
    CloseCallback closeCallback_;
    ErrorCallback errorCallback_;
};


}

#endif //TINYEV_CHANNEL_H
