set(SOURCE_FILES
        EventLoop.cc EventLoop.h
        EPoller.cc EPoller.h
        Channel.cc Channel.h
        Logger.h Logger.c
        noncopyable.h
        Acceptor.cc Acceptor.h
        InetAddress.cc InetAddress.h
        TcpConnection.cc TcpConnection.h
        Callbacks.h
        TcpServerSingle.cc TcpServerSingle.h
        TcpServer.cc TcpServer.h
        Buffer.h Buffer.cc
        ThreadPool.cc ThreadPool.h
        Connector.cc Connector.h
        TcpClient.cc TcpClient.h
        CountDownLatch.h
        EventLoopThread.cc EventLoopThread.h
        TimerQueue.cc TimerQueue.h
        Timer.h
        Timestamp.h
        )

add_library(tinyev STATIC ${SOURCE_FILES})
target_link_libraries(tinyev pthread)

install(TARGETS tinyev DESTINATION lib)
set(HEADERS
        Acceptor.h
        Buffer.h
        Callbacks.h
        Channel.h
        Connector.h
        CountDownLatch.h
        EPoller.h
        EventLoop.h
        EventLoopThread.h
        InetAddress.h
        Logger.h
        noncopyable.h
        TcpClient.h
        TcpConnection.h
        TcpServer.h
        TcpServerSingle.h
        ThreadPool.h
        Timer.h
        TimerQueue.h
        Timestamp.h
        )
install(FILES ${HEADERS} DESTINATION include)