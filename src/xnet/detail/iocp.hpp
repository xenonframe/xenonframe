#pragma once
namespace xnet
{
namespace detail
{
namespace iocp
{
	typedef detail::socket_exception socket_exception;

	class overLapped_context : OVERLAPPED
	{
	public:
		overLapped_context()
		{
			memset(this, 0, sizeof(OVERLAPPED));
		}

		enum
		{
			e_accept = 1,
			e_connect = 2,
			e_close = 4,
			e_recv = 8,
			e_send = 16,
			e_idle = 32,
			e_stop = 64,
			e_notify = 128
		};
		int status_ = e_idle;

		std::string buffer_;
		SOCKET socket_;
		WSABUF WSABuf_;
		std::size_t to_recv_len_;
		std::size_t recv_pos_;
		std::size_t recv_bytes_;
		std::size_t send_pos_;

		const int recv_some_ = 1024;
		class acceptor_impl *acceptor_ = nullptr;
		class connection_impl *connection_ = nullptr;
		class connector_impl *connector_ = nullptr;

		void reload(std::string &&data)
		{
			send_pos_ = 0;
			buffer_ = std::move(data);
			WSABuf_.buf = (CHAR*)buffer_.data();
			WSABuf_.len = (ULONG)buffer_.size();
		}
		void reload(std::size_t len)
		{
			to_recv_len_ = len;
			recv_pos_ = 0;
			recv_bytes_ = 0;
			buffer_.resize(len ? len : recv_some_);
			WSABuf_.buf = (TCHAR*)buffer_.data();
			WSABuf_.len = (ULONG)buffer_.size();
			xnet_assert(buffer_.size() == (len ? len : recv_some_));
		}
	};


	class connection_impl
	{
	public:
		using recv_callback_t = std::function<void(char*, std::size_t)>;
		using send_callback_t = std::function<void(std::size_t)>;
		connection_impl(SOCKET sock)
		{
			socket_ = sock;
			send_overlapped_ = new overLapped_context;
			send_overlapped_->connection_ = this;
			send_overlapped_->socket_ = sock;

			recv_overlapped_ = new overLapped_context;
			recv_overlapped_->connection_ = this;
			recv_overlapped_->socket_ = sock;
		}
		void set_iocp(HANDLE _iocp)
		{
			iocp_ = _iocp;
		}
		~connection_impl()
		{
			if (send_timer_id_)
				cancel_timer_(send_timer_id_);

			if (recv_timer_id_)
				cancel_timer_(recv_timer_id_);
		}
		void close()
		{
			bool close_socket = true;

			if (recv_overlapped_)
			{
				shutdown(socket_, SD_RECEIVE);
				if (recv_overlapped_->status_ ==
					overLapped_context::e_idle)
				{
					delete recv_overlapped_;
				}
				else
				{
					recv_overlapped_->status_ = overLapped_context::e_close;
					recv_overlapped_->connection_ = nullptr;
					recv_overlapped_ = nullptr;
				}
			}

			if(send_overlapped_)
			{
				if(send_overlapped_->status_ == 
					overLapped_context::e_idle)
				{
					shutdown(socket_, SD_SEND);
					close_socket = true;
					delete send_overlapped_;
					send_overlapped_ = nullptr;
				}
				else if(send_overlapped_->status_ == 
					overLapped_context::e_send)
				{
					close_socket = false;
					send_overlapped_->status_ |= 
						overLapped_context::e_close;
					send_overlapped_->connection_ = nullptr;
				}
				send_overlapped_ = nullptr;
			}
		
			if(close_socket)
				closesocket(socket_);
			delete this;
		}
		void bind_recv_callback(recv_callback_t callback)
		{
			recv_callback_ = callback;
		}
		void bind_send_callback(send_callback_t callback)
		{
			send_callback_ = callback;
		}
		int send(const char *data, int len)
		{
			return ::send(socket_, data, len, 0);
		}
		void async_send(std::string &&data)
		{
			xnet_assert(send_overlapped_->status_ ==
						overLapped_context::e_idle);
			bind_iocp();
			DWORD bytes = 0;
			send_overlapped_->reload(std::move(data));

			if(WSASend(socket_,
				&send_overlapped_->WSABuf_,
				1,
				&bytes,
				0,
				(LPOVERLAPPED)send_overlapped_,
				nullptr) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					send_timer_id_=
						set_timer_(0, [this] {
						send_callback(false);
						send_timer_id_ = 0;
						return false;
					});
				}
			}
			send_overlapped_->status_ = overLapped_context::e_send;
		}
		void async_recv(std::size_t len)
		{
			xnet_assert(recv_overlapped_->status_ ==
						overLapped_context::e_idle);
			bind_iocp();
			DWORD bytes = 0;
			DWORD flags = 0;
			recv_overlapped_->reload(len);
			if(WSARecv(
				socket_,
				&recv_overlapped_->WSABuf_,
				1,
				&bytes,
				&flags,
				(LPOVERLAPPED)recv_overlapped_,
				nullptr) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					recv_timer_id_ = 
						set_timer_(0, [this] {
						recv_callbak(false);
						recv_timer_id_ = 0;
						return false;
					});
				}
			}
			recv_overlapped_->status_ = overLapped_context::e_recv;
		}
	private:
		void bind_iocp()
		{
			if (bind_iocp_)
				return;
			bind_iocp_ = true;
			xnet_assert(::CreateIoCompletionPort((HANDLE)socket_, iocp_, 0, 0) == iocp_);
		}
		friend class proactor_impl;
		friend class acceptor_impl;
		friend class connector_impl;

		void recv_callbak(bool status)
		{
			recv_overlapped_->status_ = overLapped_context::e_idle;
			if(status)
			{
				recv_callback_((char*)recv_overlapped_->buffer_.data(),
								recv_overlapped_->recv_bytes_);
			}
			else
			{
				auto error = WSAGetLastError();
				recv_callback_(nullptr, 0);
			}
		}

		void send_callback(bool status)
		{
			send_overlapped_->status_ = overLapped_context::e_idle;
			if(status)
				send_callback_((int)send_overlapped_->buffer_.size());
			else
				send_callback_(0);
		}
		bool bind_iocp_ = false;
		recv_callback_t recv_callback_;
		send_callback_t send_callback_;
		HANDLE iocp_ = nullptr;
		overLapped_context *send_overlapped_ = nullptr;
		overLapped_context *recv_overlapped_ = nullptr;
		std::function<std::size_t(int32_t, std::function<bool()> &&)> set_timer_;
		std::function<void(std::size_t)> cancel_timer_;
		std::size_t send_timer_id_ = 0;
		std::size_t recv_timer_id_ = 0;
		SOCKET socket_ = INVALID_SOCKET;
	};
	class acceptor_impl
	{
	public:
		using accept_callback_t = std::function<void(connection_impl*)>;
		acceptor_impl()
		{
			overLapped_context_ = new overLapped_context;
			overLapped_context_->acceptor_ = this;
			overLapped_context_->status_ = overLapped_context::e_accept;
		}
		~acceptor_impl()
		{
			if(overLapped_context_)
			{
				overLapped_context_->status_ = overLapped_context::e_close;
				overLapped_context_->acceptor_ = nullptr;
				overLapped_context_->socket_ = INVALID_SOCKET;
			}
			if(socket_ != INVALID_SOCKET)
				closesocket(socket_);
			if(accept_socket_ != INVALID_SOCKET)
				closesocket(accept_socket_);
		}
		void regist_accept_callback(accept_callback_t callback)
		{
			accept_callback_ = callback;
		}
		void get_addr(std::string &ip, int &port)
		{
			sockaddr_in addr;
			int len = sizeof(addr);
			xnet_assert(!getsockname(socket_, (struct sockaddr*)&addr, &len));
			ip = inet_ntoa(addr.sin_addr);
			port = ntohs(addr.sin_port);
		}
		void bind(const std::string &ip, int port)
		{
			if(socket_ != INVALID_SOCKET)
				closesocket(socket_);
			socket_ = WSASocket(AF_INET,
										SOCK_STREAM,
										IPPROTO_TCP,
										nullptr,
										0,
										WSA_FLAG_OVERLAPPED);
			xnet_assert(socket_ != INVALID_SOCKET);

			struct sockaddr_in addr;
			ZeroMemory((char *)&addr, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(ip.c_str());
			addr.sin_port = htons(port);

			xnet_assert(::bind(socket_, (struct sockaddr *) &addr, 
				sizeof(addr)) != SOCKET_ERROR);

			xnet_assert(listen(socket_, SOMAXCONN) != SOCKET_ERROR);

			DWORD dwBytes = 0;
			GUID GuidAcceptEx = WSAID_ACCEPTEX;
			xnet_assert(WSAIoctl(socket_,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidAcceptEx,
				sizeof(GuidAcceptEx),
				&acceptex_func_,
				sizeof(acceptex_func_),
				&dwBytes,
				nullptr,
				nullptr) != SOCKET_ERROR);
			xnet_assert(acceptex_func_);

			xnet_assert(::CreateIoCompletionPort((HANDLE)socket_, IOCP_, 0, 1) == IOCP_);
			do_accept();
		}
		void close()
		{
			delete this;
		}
	private:
		friend class proactor_impl;
		void accept_callback(bool status)
		{
			if(status == false)
			{
				std::cout << "acceptor callback,error:"
					<< WSAGetLastError() << std::endl;
				return;
			}
			xnet_assert(setsockopt(accept_socket_,
				SOL_SOCKET,
				SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&socket_,
				sizeof(socket_)) == 0);
			connection_impl *conn = new connection_impl(accept_socket_);
			conn->set_iocp(IOCP_);
			conn->set_timer_ = [this](auto &&...args) {
				return timer_manager_->set_timer(std::forward<decltype(args)>(args)...);
			};
			conn->cancel_timer_ = [this](auto &&...args) {
				return timer_manager_->cancel_timer(std::forward<decltype(args)>(args)...);
			};
			xnet_assert(accept_callback_);
			accept_socket_ = INVALID_SOCKET;
			do_accept();
			accept_callback_(conn);
		}

		void do_accept()
		{
			if(accept_socket_ != INVALID_SOCKET)
				closesocket(accept_socket_);
			accept_socket_ = WSASocket(AF_INET,
										SOCK_STREAM,
										IPPROTO_TCP,
										nullptr,
										0,
										WSA_FLAG_OVERLAPPED);
			xnet_assert(accept_socket_ != INVALID_SOCKET);
			DWORD address_size = sizeof(sockaddr_in) + 16;

			if(acceptex_func_(socket_,
				accept_socket_,
				addr_buffer_,
				0,
				address_size,
				address_size,
				&bytesReceived_,
				(OVERLAPPED*)overLapped_context_) == FALSE)
			{
				xnet_assert(WSAGetLastError() == ERROR_IO_PENDING);
				return;
			}
			accept_callback(true);
		}
		char addr_buffer_[128];
		DWORD bytesReceived_ = 0;
		LPFN_ACCEPTEX acceptex_func_ = nullptr;
		SOCKET socket_ = INVALID_SOCKET;
		SOCKET accept_socket_ = INVALID_SOCKET;
		HANDLE IOCP_ = nullptr;
		timer_manager *timer_manager_;
		overLapped_context *overLapped_context_ = nullptr;
		accept_callback_t accept_callback_;
	};

	class connector_impl
	{
	public:
		using failed_callback_t = std::function<void(std::string)> ;
		using success_callback_t = std::function<void(connection_impl*)>;
		connector_impl()
		{
			overLapped_context_ = new overLapped_context;
		}
		~connector_impl()
		{
			if(socket_ != INVALID_SOCKET)
				closesocket(socket_);
		}
		void bind_success_callback(success_callback_t callback)
		{
			success_callback_ = callback;
		}
		void bind_failed_callback(failed_callback_t callback)
		{
			failed_callback_ = callback;
		}
		void sync_connect(const std::string &ip, int port)
		{
			if(socket_ != INVALID_SOCKET)
				closesocket(socket_);

			socket_ = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP
				,nullptr,0,WSA_FLAG_OVERLAPPED);
			xnet_assert(socket_ != INVALID_SOCKET);

			xnet_assert(CreateIoCompletionPort(
				reinterpret_cast<HANDLE>(socket_), iocp_, 0, 0) == iocp_);

			GUID connectex_guid = WSAID_CONNECTEX;
			DWORD bytes_returned;

			xnet_assert(WSAIoctl(socket_,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&connectex_guid,
				sizeof(connectex_guid),
				&connectex_func_,
				sizeof(connectex_func_),
				&bytes_returned, nullptr,nullptr) != SOCKET_ERROR);

			struct sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = 0;
			xnet_assert(bind(socket_,
				reinterpret_cast<SOCKADDR*>(&addr),
				sizeof(addr)) != SOCKET_ERROR);

			struct sockaddr_in socket_address;
			ZeroMemory((char *)&socket_address, sizeof(socket_address));
			socket_address.sin_family = AF_INET;
			socket_address.sin_addr.s_addr = inet_addr(ip.c_str());
			socket_address.sin_port = htons(port);

			DWORD bytes = 0;
			overLapped_context_->status_ = overLapped_context::e_connect;
			overLapped_context_->connector_ = this;

			if(!connectex_func_(socket_,
				reinterpret_cast<SOCKADDR*>(&socket_address),
				sizeof(socket_address),
				nullptr,
				0,
				&bytes,
				reinterpret_cast<LPOVERLAPPED>(overLapped_context_)))
			{
				xnet_assert(WSAGetLastError() == ERROR_IO_PENDING);
			}
		}
		void close()
		{
			if(socket_ != INVALID_SOCKET)
				closesocket(socket_);

			socket_ = INVALID_SOCKET;
			overLapped_context_->connector_ = nullptr;
			overLapped_context_->status_ = overLapped_context::e_close;
			delete this;
		}
	private:
		friend class proactor_impl;
		void connect_callback(bool result_)
		{
			if(result_ == false)
			{
				char errmsg[512] = { 0 };
				auto error_code = WSAGetLastError();
				if(!FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM,
					0, error_code, 0, errmsg, 511, nullptr))
					failed_callback_("connect failed");
				else
				{
					auto str = std::string(errmsg, strlen(errmsg));
					xnet_assert(failed_callback_);
					failed_callback_(str);
				}
				return;
			}
			SOCKET sock = socket_;
			socket_ = INVALID_SOCKET;
			auto conn = new connection_impl(sock);
			conn->set_timer_ = [this](auto &&...args) {
				return timer_manager_->set_timer(std::forward<decltype(args)>(args)...);
			};
			conn->cancel_timer_ = [this](auto &&...args) {
				return timer_manager_->cancel_timer(std::forward<decltype(args)>(args)...);
			};
			success_callback_(conn);
		}
		overLapped_context *overLapped_context_;
		timer_manager *timer_manager_;
		LPFN_CONNECTEX connectex_func_ = nullptr;
		SOCKET socket_ = INVALID_SOCKET;
		success_callback_t success_callback_;
		failed_callback_t failed_callback_;
		HANDLE iocp_ = nullptr;
	};

	class proactor_impl
	{
	public:
		proactor_impl()
		{

		}
		void init()
		{
			static std::once_flag once;
			std::call_once(once, []
			{
				WSADATA wsaData;
				int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
				xnet_assert(NO_ERROR == nResult);
			});
			iocp_ = CreateIoCompletionPort(
				INVALID_HANDLE_VALUE, nullptr, 0, 1);
			xnet_assert(iocp_);
		}
		void run()
		{
			while(is_stop_ == false)
			{
				void *key = nullptr;
				overLapped_context *overlapped = nullptr;
				DWORD bytes = 0;
				DWORD timeout = (DWORD)timer_manager_.do_timer();
				timeout = timeout ? timeout : 1000;
				BOOL status = GetQueuedCompletionStatus(
					iocp_,
					&bytes,
					(PULONG_PTR)&key,
					(LPOVERLAPPED*)&overlapped,
					timeout);

				if(status == FALSE)
				{
					if(GetLastError() == WAIT_TIMEOUT)
						continue;
				}
				if(overlapped->status_ == overLapped_context::e_stop)
				{
					delete overlapped;
					break;
				}
				xnet_assert(overlapped);
				try
				{
					if (status == FALSE)
						handle_completion_failed(overlapped, bytes);
					else
						handle_completion_success(overlapped, bytes);
				}
				catch (const std::exception& e)
				{
					std::cout << e.what() << std::endl;
				}
			}
		}
		void stop()
		{
			is_stop_ = true;
			overLapped_context *overlapped = new overLapped_context;
			overlapped->status_ = overLapped_context::e_stop;
			PostQueuedCompletionStatus(iocp_,0,(ULONG_PTR)this, 
				(LPOVERLAPPED)overlapped);
		}
		acceptor_impl *get_acceptor()
		{
			acceptor_impl *obj = new acceptor_impl;
			xnet_assert(iocp_);
			obj->IOCP_ = iocp_;
			obj->timer_manager_ = &timer_manager_;
			return obj;
		}
		connector_impl *get_connector()
		{
			connector_impl *obj = new connector_impl;
			xnet_assert(iocp_);
			obj->iocp_ = iocp_;
			obj->timer_manager_ = &timer_manager_;
			return obj;
		}
		template<typename T>
		timer_id set_timer(std::size_t timeout, T &&timer_callback)
		{
			return timer_manager_.set_timer(timeout, std::forward<T>(timer_callback));
		}
		void cancel_timer(timer_id id)
		{
			timer_manager_.cancel_timer(id);
		}
		void regist_connection(connection_impl &conn)
		{
			xnet_assert(!conn.bind_iocp_);
			conn.iocp_= iocp_;
			conn.set_timer_ = [this](int32_t timeout, std::function<bool()> &&action) {
				return set_timer(timeout, std::move(action));
			};
			conn.cancel_timer_ = [this](std::size_t timer_id) {
				return cancel_timer(timer_id);
			};
		}
	private:
		void continue_send(overLapped_context *overlapped, DWORD bytes)
		{
			DWORD dwBytes = 0;

			overlapped->send_pos_ += bytes;

			overlapped->WSABuf_.buf =
				(char *)overlapped->buffer_.data() +
				overlapped->send_pos_;

			overlapped->WSABuf_.len = 
				static_cast<ULONG>(
					overlapped->buffer_.size() -overlapped->send_pos_);

			if(WSASend(overlapped->socket_,
				&overlapped->WSABuf_,
				1,
				&dwBytes,
				0,
				(LPOVERLAPPED)overlapped,
				nullptr) == SOCKET_ERROR)
			{
				xnet_assert(WSAGetLastError() == WSA_IO_PENDING);
			}
		}
		void do_recv(overLapped_context * overlapped, DWORD bytes)
		{
			DWORD dwBytes = 0,
				dwFlags = 0;
			overlapped->WSABuf_.buf =
				(char*)overlapped->buffer_.data() +
				overlapped->recv_pos_;

			overlapped->WSABuf_.len = static_cast<ULONG>(
				overlapped->to_recv_len_ -overlapped->recv_pos_);

			if(WSARecv(
				overlapped->socket_,
				&overlapped->WSABuf_,
				1,
				&dwBytes,
				&dwFlags,
				(LPOVERLAPPED)overlapped,
				nullptr) == SOCKET_ERROR)
			{
				xnet_assert(WSAGetLastError() == WSA_IO_PENDING);
			}
		}
		void handle_completion_failed(overLapped_context * overlapped, DWORD bytes)
		{
			if(overlapped->status_ == overLapped_context::e_recv)
			{
				overlapped->connection_->recv_callbak(false);
			}
			else if(overlapped->status_ == overLapped_context::e_send)
			{
				overlapped->connection_->send_callback(false);
			}
			else if(overlapped->status_ == overLapped_context::e_accept)
			{
				overlapped->acceptor_->accept_callback(false);
			}
			else if(overlapped->status_ & overLapped_context::e_close &&
					overlapped->status_ & overLapped_context::e_send)
			{
				closesocket(overlapped->socket_);
				delete overlapped;
			}
			else if(overlapped->status_ == overLapped_context::e_close)
			{
				if(overlapped->socket_ != INVALID_SOCKET)
					closesocket(overlapped->socket_);
				delete overlapped;

			}
			else if(overlapped->status_ == overLapped_context::e_connect)
			{
				overlapped->connector_->connect_callback(false);
			}
		}

		void handle_completion_success(overLapped_context * overlapped, DWORD bytes)
		{
			if(overlapped->status_ == overLapped_context::e_recv)
			{
				overlapped->recv_bytes_ += bytes;
				overlapped->recv_pos_ += bytes;
				if (overlapped->to_recv_len_ > overlapped->recv_bytes_ && bytes > 0)
					do_recv(overlapped, bytes);
				else if(bytes <= 0 || overlapped->to_recv_len_ <= overlapped->recv_bytes_)
				{
					overlapped->connection_->recv_callbak(!!bytes);
				}

			}
			else if(overlapped->status_ == overLapped_context::e_send)
			{
				if(overlapped->WSABuf_.len > bytes  && bytes > 0)
					continue_send(overlapped, bytes);
				else
					overlapped->connection_->send_callback(true);

			}
			else if(overlapped->status_ == overLapped_context::e_accept)
			{
				overlapped->acceptor_->accept_callback(true);

			}
			else if(overlapped->status_ & overLapped_context::e_close &&
					overlapped->status_ & overLapped_context::e_send)
			{
				if(overlapped->WSABuf_.len > bytes)
				{
					continue_send(overlapped, bytes);
				}
				else
				{
					shutdown(overlapped->socket_, SD_SEND);
					closesocket(overlapped->socket_);
					delete overlapped;
				}
			}

			else if(overlapped->status_ == overLapped_context::e_close)
			{
				if(overlapped->socket_ != INVALID_SOCKET)
					closesocket(overlapped->socket_);
				delete overlapped;
			}
			else if(overlapped->status_ == overLapped_context::e_connect)
			{
				overlapped->connector_->connect_callback(true);
			}
		}
		timer_manager timer_manager_;
		bool is_stop_ = false;
		HANDLE iocp_ = nullptr;
	};
}

typedef iocp::connection_impl connection_impl;
typedef iocp::acceptor_impl acceptor_impl;
typedef iocp::proactor_impl proactor_impl;
typedef iocp::connector_impl connector_impl;
typedef iocp::socket_exception socket_exception;
}
}