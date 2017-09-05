cmake_minimum_required(VERSION 3.8)
project(tinyev)

set(CMAKE_CXX_STANDARD 14)

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
		Connector.cc Connector.h TcpClient.cc TcpClient.h)
add_subdirectory(example)
add_compile_options(-Wall -Wextra -Werror)
add_library(tinyev STATIC ${SOURCE_FILES})
target_link_libraries(tinyev pthread)