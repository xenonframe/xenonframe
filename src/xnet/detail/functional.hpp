#pragma once
namespace xnet
{
	namespace detail
	{
#ifdef _MSC_VER
		struct _socket_closer
		{
			void operator()(SOCKET s)
			{
				closesocket(s);
			}
		};
		struct _socket_initer
		{
			static _socket_initer &get_instance()
			{
				WSADATA wsaData;
				int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (NO_ERROR != nResult)
				{
					xnet_assert(false);
				}
				static _socket_initer inst;
				return inst;
			}
			~_socket_initer()
			{
				WSACleanup();
			}
		};
		struct _selecter
		{
			int operator()(SOCKET maxfds,fd_set *readfds,
				fd_set *writefds,fd_set *exceptfds,
				std::size_t timeout)
			{
				struct timeval tv = {
					(long)(timeout / 1000),
					(long)(timeout % 1000 * 1000)
				};
				int rc = ::select(0,
								  readfds,
								  writefds, 
								  exceptfds,
								  timeout ? &tv : NULL);
				xnet_assert(rc != SOCKET_ERROR);
				return rc;
			}
		};
		struct _get_last_errorer
		{
			int operator()()
			{
				return GetLastError();
			}
		};
		struct setnonblocker
		{
			int operator()(SOCKET sock)
			{
				u_long nonblock = 1;
				return ioctlsocket(sock, FIONBIO, &nonblock);
			}
		};

		struct setblocker
		{
			int operator()(SOCKET sock)
			{
				u_long nonblock = 0;
				return ioctlsocket(sock, FIONBIO, &nonblock);
			}
		};
#else
		struct setnonblocker
		{
			int operator()(int sock)
			{
				int flags = fcntl(sock, F_GETFL, 0);
				if(flags == -1)
					flags = 0;
				return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
			}
		};
		struct setblocker
		{
			int operator()(int sock)
			{
				int flags = fcntl(sock, F_GETFL, 0);
				if (flags == -1)
					flags = 0;
				return fcntl(sock, F_SETFL, flags &~O_NONBLOCK);
			}
		};

		struct _socket_closer
		{
			void operator()(int s)
			{
				close(s);
			}
		};
		struct _socket_initer
		{
			static _socket_initer &get_instance()
			{
				static _socket_initer inst;
				return inst;
			}
			~_socket_initer(){}
		};
		struct _selecter
		{
			int operator()(
				SOCKET maxfds,
				fd_set *readfds,
				fd_set *writefds,
				fd_set *exceptfds,
				std::size_t timeout)
			{
				struct timeval tv = {
					(long)(timeout / 1000),
					(long)(timeout % 1000 * 1000)
				};
				int rc = ::select(maxfds + 1, 
								  readfds, 
								  writefds, 
								  exceptfds, 
								  timeout ? &tv : NULL);
				if (rc == -1) {
					xnet_assert(errno == EINTR);
					return 0;
				}
				return rc;
			}
		};
		struct _get_last_errorer
		{
			int operator()()
			{
				return errno;
			}
		};
#endif
	}

	typedef detail::_socket_closer socket_closer;
	typedef detail::_socket_initer socket_initer;
	typedef detail::_selecter selecter;
	typedef detail::_get_last_errorer get_last_errorer;
	typedef detail::setnonblocker setnonblocker;
}