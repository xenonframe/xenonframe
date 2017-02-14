#pragma once
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <cassert>
#include <exception>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <string>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <malloc.h>
#include "timer.hpp"

#if defined _MSC_VER
#undef FD_SETSIZE
#define FD_SETSIZE      1024
#ifndef IOCP
#define IOCP 1
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include "exceptions.hpp"
#include "functional.hpp"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#else
#define _LINUX_
#define EPOLL 1
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#ifndef MAXIOEVENTS
#define MAXIOEVENTS 256
#endif
#define  SD_SEND SHUT_WR 
#define  SD_RECEIVE SHUT_RD 
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEINPROGRESS EINPROGRESS
#include "exceptions.hpp"
#include "functional.hpp"
#endif
#if IOCP
#include "iocp.hpp"
#elif EPOLL 
#include "epoll.hpp"
#else
#include "select.hpp"
#endif
