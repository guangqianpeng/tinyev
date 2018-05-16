//
// Created by frank on 17-9-1.
//

#include <unistd.h>
#include <cassert>

#include <tinyev/EventLoop.h>
#include <tinyev/Logger.h>
#include <tinyev/InetAddress.h>
#include <tinyev/Acceptor.h>

using namespace ev;

namespace
{

int createSocket()
{
    int ret = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (ret == -1)
        SYSFATAL("Acceptor::socket()");
    return ret;
}

}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& local)
        : listening_(false),
          loop_(loop),
          acceptFd_(createSocket()),
          acceptChannel_(loop, acceptFd_),
          local_(local)
{
    int on = 1;
    int ret = ::setsockopt(acceptFd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret == -1)
        SYSFATAL("Acceptor::setsockopt() SO_REUSEADDR");
    ret = ::setsockopt(acceptFd_, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    if (ret == -1)
        SYSFATAL("Acceptor::setsockopt() SO_REUSEPORT");
    ret = ::bind(acceptFd_, local.getSockaddr(), local.getSocklen());
    if (ret == -1)
        SYSFATAL("Acceptor::bind()");
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    int ret = ::listen(acceptFd_, SOMAXCONN);
    if (ret == -1)
        SYSFATAL("Acceptor::listen()");

    acceptChannel_.setReadCallback([this](){handleRead();});
    acceptChannel_.enableRead();
}


Acceptor::~Acceptor()
{
    ::close(acceptFd_);
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    void* any = &addr;
    int sockfd = ::accept4(acceptFd_, static_cast<sockaddr*>(any),
                           &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (sockfd == -1) {
        int savedErrno = errno;
        SYSERR("Acceptor::accept4()");
        switch (savedErrno) {
            case ECONNABORTED:
            case EMFILE:
                break;
            default:
                FATAL("unexpected accept4() error");
        }
    }

    if (newConnectionCallback_) {
        InetAddress peer;
        peer.setAddress(addr);
        newConnectionCallback_(sockfd, local_, peer);
    }
    else ::close(sockfd);
}