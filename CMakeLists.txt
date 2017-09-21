cmake_minimum_required(VERSION 3.5)
project(tinyev)

set(CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
		-Wall
		-Wextra
		-Werror
		-Wconversion
		-Wno-unused-parameter
		-Wold-style-cast
		-Woverloaded-virtual
		-Wpointer-arith
		-Wshadow
		-Wwrite-strings)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

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
		EventLoopThread.cc EventLoopThread.h)

add_subdirectory(kth_element)
add_subdirectory(nqueen)
add_subdirectory(trivial)

add_library(tinyev STATIC ${SOURCE_FILES})
target_link_libraries(tinyev pthread)