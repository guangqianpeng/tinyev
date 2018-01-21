//
// Created by frank on 17-8-31.
//

#ifndef TINYEV_EPOLLER_H
#define TINYEV_EPOLLER_H

#include <vector>

#include <tinyev/noncopyable.h>

namespace ev
{

class EventLoop;
class Channel;

class EPoller: noncopyable
{
public:
    typedef std::vector<Channel*> ChannelList;

    explicit
    EPoller(EventLoop* loop);
    ~EPoller();

    void poll(ChannelList& activeChannels);
    void updateChannel(Channel* channel);

private:
    void updateChannel(int op, Channel* channel);
    EventLoop* loop_;
    std::vector<struct epoll_event> events_;
    int epollfd_;
};

}

#endif //TINYEV_EPOLLER_H
