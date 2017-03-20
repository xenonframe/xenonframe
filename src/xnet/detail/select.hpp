#pragma once
namespace xnet
{
namespace detail
{
namespace select
{
	typedef detail::socket_exception socket_exception;
	class io_context
	{
	public:
		io_context()
		{
		}
		~io_context()
		{
		}
		void reload(std::string&& data)
		{
			to_send_ = (std::size_t)data.size();
			send_bytes_ = 0;
			buffer_ = std::move(data);
		}
		void reload(std::size_t len)
		{
			to_recv_ = len;
			recv_bytes_ = 0;
			buffer_.resize(1 + (len ? len : recv_some_));
		}

		enum status_t
		{
			e_recv = 1,
			e_send = 2,
			e_connect = 4,
			e_accept = 8,
			e_close = 16,
			e_idle = 32,
			e_stop = 64

		};
		int status_ = e_idle;
		int last_status_ = e_idle;

		SOCKET socket_ = INVALID_SOCKET;
		std::string buffer_;
		std::size_t to_recv_;
		std::size_t recv_bytes_;

		std::size_t to_send_;
		std::size_t send_bytes_;

		const int recv_some_ = 1024;

		class connection_impl *connection_ = nullptr;
		class acceptor_impl *acceptor_ = nullptr;
		class connector_impl *connector_ = nullptr;
	};

	class connection_impl
	{
	public:
		using recv_callback_t = std::function<void(char*, std::size_t)>;
		using send_callback_t = std::function<void(std::size_t)>;

		connection_impl(SOCKET _socket)
			:socket_(_socket)
		{
		}
		~connection_impl()
		{

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
			xnet_assert(send_ctx_->status_ == io_context::e_idle);
			send_ctx_->reload(std::move(data));
			send_ctx_->status_ = io_context::e_send;
			if (send_ctx_->last_status_ == io_context::e_send)
				return;
			xnet_assert(regist_send_ctx_);
			regist_send_ctx_(send_ctx_);
		}
		void async_recv(std::size_t len)
		{
			xnet_assert(recv_ctx_->status_ == io_context::e_idle);
			recv_ctx_->reload(len);
			recv_ctx_->status_ = io_context::e_recv;
			if (recv_ctx_->last_status_ == io_context::e_recv)
				return;
			xnet_assert(regist_recv_ctx_);
			regist_recv_ctx_(recv_ctx_);
		}
		void close()
		{
			if (send_ctx_->status_ == io_context::e_idle)
			{
				del_io_context_(recv_ctx_);
			}
			else if (send_ctx_->status_ == io_context::e_send)
			{
				send_ctx_->status_ |= io_context::e_close;
			}
			if (!!in_callback_)
				close_flag_ = true;
			else
				delete this;
		}

	private:
		friend class proactor_impl;
		friend class acceptor_impl;
		friend class connector_impl;
		void init()
		{
			send_ctx_ = new io_context;
			recv_ctx_ = new io_context;
			xnet_assert(send_ctx_);
			xnet_assert(recv_ctx_);
			send_ctx_->connection_ = this;
			send_ctx_->socket_ = socket_;
			recv_ctx_->connection_ = this;
			recv_ctx_->socket_ = socket_;
		}
		void send_callback(bool status)
		{
			send_ctx_->last_status_ = send_ctx_->status_;
			send_ctx_->status_ = io_context::e_idle;
			++in_callback_;
			if (status)
				send_callback_(send_ctx_->send_bytes_);
			else
				send_callback_(0);
			--in_callback_;
			if (!close_flag_ && send_ctx_->status_ != io_context::e_send)
			{
				send_ctx_->last_status_ = io_context::e_idle;
				unregist_send_ctx_(send_ctx_);
			}
			if (close_flag_)
				delete  this;
		}
		void recv_callback(bool status)
		{
			recv_ctx_->last_status_ = recv_ctx_->status_;
			recv_ctx_->status_ = io_context::e_idle;
			recv_ctx_->buffer_.push_back('\0');
			++in_callback_;
			if (status)
				recv_callback_((char*)recv_ctx_->buffer_.data(),
					recv_ctx_->recv_bytes_);
			else
				recv_callback_(nullptr, 0);
			--in_callback_;
			if (!close_flag_ && recv_ctx_->status_ != io_context::e_recv)
			{
				recv_ctx_->last_status_ = io_context::e_idle;
				unregist_recv_ctx_(recv_ctx_);
			}
			if (close_flag_)
				delete  this;
		}
		SOCKET socket_ = INVALID_SOCKET;
		bool close_flag_ = false;
		int in_callback_ = false;
		io_context *send_ctx_ = nullptr;
		io_context *recv_ctx_ = nullptr;

		std::function<void(io_context*)> regist_send_ctx_;
		std::function<void(io_context*)> unregist_send_ctx_;
		std::function<void(io_context*)> regist_recv_ctx_;
		std::function<void(io_context*)> unregist_recv_ctx_;
		std::function<void(io_context*)> del_io_context_;

		recv_callback_t recv_callback_;
		send_callback_t send_callback_;
	};
	class acceptor_impl
	{
	public:
		using accept_callback_t = std::function<void(connection_impl*)>;
		acceptor_impl()
		{

		}
		void regist_accept_callback(accept_callback_t callback)
		{
			acceptor_callback_ = callback;
		}
		void bind(const std::string &ip, int port)
		{
			socket_ = socket(AF_INET, SOCK_STREAM, 0);
			xnet_assert(socket_ != INVALID_SOCKET);

			setnonblocker nonblocker;
			xnet_assert(nonblocker(socket_) != INVALID_SOCKET);

			int nodelay = 1;
			xnet_assert(setsockopt(socket_,
				IPPROTO_TCP,
				TCP_NODELAY,
				(char*)&nodelay,
				sizeof(int)) != -1);

			int flags = 1;
			xnet_assert(setsockopt(socket_, 
				SOL_SOCKET, 
				SO_REUSEADDR, 
				(char*)&flags, 
				sizeof(int)) == 0);

			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = inet_addr(ip.c_str());

			xnet_assert(!::bind(socket_, (struct sockaddr*)&addr, sizeof(addr)));
			xnet_assert(!listen(socket_, SOMAXCONN));

			accept_ctx_ = new io_context;
			accept_ctx_->acceptor_ = this;
			accept_ctx_->socket_ = socket_;
			accept_ctx_->status_ = io_context::e_accept;
			xnet_assert(regist_accept_ctx_);
			regist_accept_ctx_(accept_ctx_);
		}
		void get_addr(std::string &ip, int &port)
		{
			sockaddr_in addr;
			socklen_t len = sizeof(addr);
			xnet_assert(!getsockname(socket_, (struct sockaddr*)&addr, &len));
			ip = inet_ntoa(addr.sin_addr);
			port = ntohs(addr.sin_port);
		}
		void close()
		{
			del_io_context_(accept_ctx_);
			delete this;
		}
	private:
		friend class proactor_impl;

		void on_accept(bool result)
		{
			xnet_assert(result);
			do
			{
				SOCKET sock = ::accept(socket_, nullptr, nullptr);
				if (sock == INVALID_SOCKET)
					return;
				auto conn = new connection_impl(sock);
				conn->init();
				xnet_assert(conn);
				conn->regist_recv_ctx_ = regist_recv_ctx_;
				conn->unregist_recv_ctx_ = unregist_recv_ctx_;
				conn->regist_send_ctx_ = regist_send_ctx_;
				conn->unregist_send_ctx_ = unregist_send_ctx_;
				conn->del_io_context_ = del_io_context_;
				acceptor_callback_(conn);

			} while (true);
		}

		SOCKET socket_ = INVALID_SOCKET;
		io_context* accept_ctx_ = nullptr;
		std::function<void(io_context*)> regist_accept_ctx_;
		std::function<void(io_context*)> regist_send_ctx_;
		std::function<void(io_context*)> unregist_send_ctx_;
		std::function<void(io_context*)> regist_recv_ctx_;
		std::function<void(io_context*)> unregist_recv_ctx_;
		std::function<void(io_context*)> del_io_context_;
		accept_callback_t acceptor_callback_;
	};
	class connector_impl
	{
	public:
		using success_callback_t = std::function<void(connection_impl *)>;
		using failed_callback_t = std::function<void(std::string)>;

		connector_impl() {}

		~connector_impl()
		{ }
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
			socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			xnet_assert(socket_ != INVALID_SOCKET);
			xnet_assert(!setnonblocker()(socket_));
			reset_connect_context();
			connect_ctx_ = new io_context;
			xnet_assert(connect_ctx_);
			connect_ctx_->connector_ = this;
			connect_ctx_->socket_ = socket_;
			connect_ctx_->status_ = io_context::e_connect;

			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(ip.c_str());
			addr.sin_port = htons(port);

			int res = connect(socket_, (struct sockaddr*)&addr, sizeof(addr));
			if (res == 0)
			{
				on_connect(true);
				return;
			}
			xnet_assert(regist_accept_ctx_);
			regist_accept_ctx_(connect_ctx_);
		}
		void close()
		{
			reset_connect_context();
			delete this;
		}
	private:
		friend class proactor_impl;

		void reset_connect_context()
		{
			if (connect_ctx_)
			{
				connect_ctx_->status_ = io_context::e_close;
				connect_ctx_->connector_ = nullptr;
				del_io_context_(connect_ctx_);
				connect_ctx_ = nullptr;
			}
		}
		void on_connect(bool result)
		{
			xnet_assert(failed_callback_);
			xnet_assert(success_callback_);
			if (!result)
			{
				reset_connect_context();
				return failed_callback_("connect failed");
			}

			int err = 0;
			socklen_t len = sizeof(err);
			xnet_assert(!getsockopt(socket_,
				SOL_SOCKET, SO_ERROR, (char*)&err, &len));
			if (err == 0)
			{
				unregist_send_ctx_(connect_ctx_);
				xnet_assert(!setblocker()(socket_));
				auto conn = new connection_impl(socket_);
				connect_ctx_->connector_ = nullptr;
				conn->send_ctx_ = connect_ctx_;
				conn->recv_ctx_ = new io_context;
				conn->send_ctx_->connection_ = conn;
				conn->recv_ctx_->connection_ = conn;
				conn->recv_ctx_->socket_ = socket_;
				conn->send_ctx_->socket_ = socket_;
				conn->send_ctx_->status_ = io_context::e_idle;
				conn->recv_ctx_->status_ = io_context::e_idle;

				conn->regist_send_ctx_ = regist_send_ctx_;
				conn->unregist_send_ctx_ = unregist_send_ctx_;
				conn->regist_recv_ctx_ = regist_recv_ctx_;
				conn->unregist_recv_ctx_ = unregist_recv_ctx_;
				conn->del_io_context_ = del_io_context_;

				socket_ = INVALID_SOCKET;
				connect_ctx_ = nullptr;
				success_callback_(conn);
			}
			else
			{
				return failed_callback_("connect failed");
			}
		}
		SOCKET socket_ = INVALID_SOCKET;
		io_context *connect_ctx_ = nullptr;

		std::function<void(io_context*)> regist_send_ctx_;
		std::function<void(io_context*)> unregist_send_ctx_;
		std::function<void(io_context*)> regist_recv_ctx_;
		std::function<void(io_context*)> unregist_recv_ctx_;
		std::function<void(io_context*)> del_io_context_;

		std::function<void(io_context*)> regist_accept_ctx_;

		success_callback_t success_callback_;
		failed_callback_t failed_callback_;
	};

	class proactor_impl
	{
	private:
		struct fd_context
		{
			SOCKET socket_ = INVALID_SOCKET;
			bool del_flag_ = false;
			io_context *recv_ctx_ = nullptr;
			io_context *send_ctx_ = nullptr;
		};
		typedef std::vector <fd_context> fd_context_vec;

	public:
		proactor_impl()
		{

		}
		void init()
		{
			socket_initer::get_instance();
			FD_ZERO(&source_recv_fds_);
			FD_ZERO(&source_send_fds_);
			FD_ZERO(&source_except_fds_);
		}
		void run()
		{
			while (!is_stop_)
			{
				memcpy(&recv_fds_, &source_recv_fds_, sizeof(source_recv_fds_));
				memcpy(&send_fds_, &source_send_fds_, sizeof(source_send_fds_));
				memcpy(&except_fds_, &source_except_fds_, sizeof(source_except_fds_));

				int64_t timeout = timer_manager_.do_timer();
				timeout = timeout > 0 ? timeout : 1000;

				int rc = selecter_(maxfd_, &recv_fds_, &send_fds_,
					&except_fds_, (std::size_t)timeout);

				if (rc == 0)
					continue;
				try
				{
					for (fd_context_vec::size_type i = 0; i < fd_ctxs_.size(); i++)
					{
						if (fd_ctxs_[i].socket_ == INVALID_SOCKET)
							continue;
						if (FD_ISSET(fd_ctxs_[i].socket_, &except_fds_))
							except_callback(fd_ctxs_[i]);
						if (fd_ctxs_[i].socket_ == INVALID_SOCKET)
							continue;
						if (FD_ISSET(fd_ctxs_[i].socket_, &send_fds_))
							writeable_callback(fd_ctxs_[i]);
						if (fd_ctxs_[i].socket_ == INVALID_SOCKET)
							continue;
						if (FD_ISSET(fd_ctxs_[i].socket_, &recv_fds_))
							readable_callback(fd_ctxs_[i]);
					}
				}
				catch (const std::exception&e)
				{
					std::cout << e.what() << std::endl;
				}

				if (retired)
				{
					fd_ctxs_.erase(std::remove_if(fd_ctxs_.begin(),
						fd_ctxs_.end(),
						[](fd_context &entry)
					{
						if (entry.socket_ == INVALID_SOCKET)
						{
							if (entry.recv_ctx_)
								delete entry.recv_ctx_;
							if (entry.send_ctx_)
								delete entry.send_ctx_;
							return true;
						}
						return false;
					}), fd_ctxs_.end());
					retired = false;
				}
			}
		}
		void stop()
		{
			is_stop_ = true;
		}
		acceptor_impl *get_acceptor()
		{
			acceptor_impl *acceptor = new acceptor_impl;

			acceptor->regist_accept_ctx_ = std::bind(
				&proactor_impl::regist_recv_context,
				this, std::placeholders::_1);

			acceptor->regist_recv_ctx_ = std::bind(
				&proactor_impl::regist_recv_context,
				this, std::placeholders::_1);

			acceptor->unregist_recv_ctx_ = std::bind(
				&proactor_impl::unregist_recv_context,
				this, std::placeholders::_1);

			acceptor->regist_send_ctx_ = std::bind(
				&proactor_impl::regist_send_context,
				this, std::placeholders::_1);

			acceptor->unregist_send_ctx_ = std::bind(
				&proactor_impl::unregist_send_context,
				this, std::placeholders::_1);

			acceptor->del_io_context_ = std::bind(
				&proactor_impl::del_io_context,
				this, std::placeholders::_1);
			return acceptor;
		}

		connector_impl *get_connector()
		{
			connector_impl *connector = new connector_impl;

			connector->regist_accept_ctx_ = std::bind(
				&proactor_impl::regist_send_context,
				this, std::placeholders::_1);

			connector->regist_recv_ctx_ = std::bind(
				&proactor_impl::regist_recv_context,
				this, std::placeholders::_1);

			connector->unregist_recv_ctx_ = std::bind(
				&proactor_impl::unregist_recv_context,
				this, std::placeholders::_1);

			connector->regist_send_ctx_ = std::bind(
				&proactor_impl::regist_send_context,
				this, std::placeholders::_1);

			connector->unregist_send_ctx_ = std::bind(
				&proactor_impl::unregist_send_context,
				this, std::placeholders::_1);

			connector->del_io_context_ = std::bind(
				&proactor_impl::del_io_context,
				this, std::placeholders::_1);
			return connector;
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
			conn.regist_recv_ctx_ = std::bind(
				&proactor_impl::regist_recv_context,
				this, std::placeholders::_1);

			conn.unregist_recv_ctx_ = std::bind(
				&proactor_impl::unregist_recv_context,
				this, std::placeholders::_1);

			conn.regist_send_ctx_ = std::bind(
				&proactor_impl::regist_send_context,
				this, std::placeholders::_1);

			conn.unregist_send_ctx_ = std::bind(
				&proactor_impl::unregist_send_context,
				this, std::placeholders::_1);

			conn.del_io_context_ = std::bind(
				&proactor_impl::del_io_context,
				this, std::placeholders::_1);
		}
	private:
		void regist_recv_context(io_context *io_ctx_)
		{
			for (auto &itr : fd_ctxs_)
			{
				if (itr.socket_ == io_ctx_->socket_)
				{
					itr.recv_ctx_ = io_ctx_;
					if (io_ctx_->socket_ > maxfd_)
						maxfd_ = io_ctx_->socket_;
					FD_SET(itr.socket_, &source_recv_fds_);
					return;
				}
			}
			fd_context fd_ctx;
			fd_ctx.socket_ = io_ctx_->socket_;
			fd_ctx.recv_ctx_ = io_ctx_;
			fd_ctxs_.push_back(fd_ctx);
			FD_SET(io_ctx_->socket_, &source_recv_fds_);
			FD_SET(io_ctx_->socket_, &source_except_fds_);
			if (io_ctx_->socket_ > maxfd_)
				maxfd_ = io_ctx_->socket_;
		}
		void unregist_recv_context(io_context *io_ctx_)
		{
			FD_CLR(io_ctx_->socket_, &source_recv_fds_);
		}

		void regist_send_context(io_context *io_ctx_)
		{
			for (auto &itr : fd_ctxs_)
			{
				if (itr.socket_ == io_ctx_->socket_)
				{
					itr.send_ctx_ = io_ctx_;
					FD_SET(itr.socket_, &source_send_fds_);
					if (io_ctx_->socket_ > maxfd_)
						maxfd_ = io_ctx_->socket_;
					return;
				}
			}
			fd_context fd_ctx;
			fd_ctx.socket_ = io_ctx_->socket_;
			fd_ctx.send_ctx_ = io_ctx_;
			fd_ctxs_.push_back(fd_ctx);
			FD_SET(io_ctx_->socket_, &source_send_fds_);
			FD_SET(io_ctx_->socket_, &source_except_fds_);
			if (io_ctx_->socket_ > maxfd_)
				maxfd_ = io_ctx_->socket_;
		}
		void unregist_send_context(io_context *io_ctx_)
		{
			FD_CLR(io_ctx_->socket_, &source_send_fds_);
		}
		void del_io_context(io_context *io_ctx_)
		{
			for (auto &itr : fd_ctxs_)
			{
				if (itr.socket_ == io_ctx_->socket_)
				{
					if (itr.socket_ > maxfd_)
						maxfd_ = INVALID_SOCKET;
					FD_CLR(itr.socket_, &source_send_fds_);
					FD_CLR(itr.socket_, &source_recv_fds_);
					FD_CLR(itr.socket_, &source_except_fds_);
					FD_CLR(itr.socket_, &send_fds_);
					FD_CLR(itr.socket_, &recv_fds_);
					FD_CLR(itr.socket_, &except_fds_);
					socket_closer_(itr.socket_);
					itr.socket_ = INVALID_SOCKET;
					retired = true;
					break;
				}
			}
			if (maxfd_ == INVALID_SOCKET)
			{
				for (auto &itr : fd_ctxs_)
					if (itr.socket_ > maxfd_)
						maxfd_ = itr.socket_;
			}
				
		}
		void readable_callback(fd_context& fd_ctx)
		{
			if (!fd_ctx.recv_ctx_)
				return;

			io_context &io_ctx = *fd_ctx.recv_ctx_;
			if (io_ctx.status_ == io_context::e_recv)
			{
				std::size_t len = io_ctx.to_recv_? 
					io_ctx.to_recv_: io_ctx.buffer_.size();

				auto bytes = ::recv(io_ctx.socket_,
					(char*)io_ctx.buffer_.data() + io_ctx.recv_bytes_,
					(int)(len - io_ctx.recv_bytes_), 0);
#ifdef _MSC_VER
				if (bytes == -1 && (WSAGetLastError() != WSAEWOULDBLOCK))
#else
				if (bytes == -1 && !(errno == EWOULDBLOCK ||
					errno == EINTR))
#endif // _MSC_VER
				{
					io_ctx.connection_->recv_callback(false);
					return;
				}
				io_ctx.recv_bytes_ += bytes;
				if (io_ctx.to_recv_ <= io_ctx.recv_bytes_)
				{
					io_ctx.connection_->recv_callback(true);
				}
			}
			else if (io_ctx.status_ == io_context::e_accept)
			{
				io_ctx.acceptor_->on_accept(true);
			}
			else if (io_ctx.status_ == io_context::e_connect)
			{
				io_ctx.connector_->on_connect(true);
			}
		}
		void writeable_callback(fd_context& fd_ctx)
		{
			if (!fd_ctx.send_ctx_)
				return;
			io_context &io_ctx = *fd_ctx.send_ctx_;

			if (io_ctx.status_ == io_context::e_send)
			{
				auto bytes = ::send(io_ctx.socket_,
					(char*)io_ctx.buffer_.data() + io_ctx.send_bytes_,
					static_cast<int>(io_ctx.to_send_ - io_ctx.send_bytes_),
					0);

#ifdef _MSC_VER
				if (bytes == -1 && (WSAGetLastError() != WSAEWOULDBLOCK))
#else
				if (bytes == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK ||
					errno == EINTR))
#endif // _MSC_VER
				{
					io_ctx.connection_->send_callback(false);
					return;
				}
				io_ctx.send_bytes_ += bytes;
				if (io_ctx.to_send_ == io_ctx.send_bytes_)
				{
					io_ctx.connection_->send_callback(true);
				}
			}
			else if (io_ctx.status_ & io_context::e_send &&
				io_ctx.status_ & io_context::e_close)
			{
				auto bytes = ::send(io_ctx.socket_,
					(char*)io_ctx.buffer_.data() + io_ctx.send_bytes_,
					static_cast<int>(io_ctx.to_send_ - io_ctx.send_bytes_), 
					0);
#ifdef _MSC_VER
				if(bytes == -1 && (WSAGetLastError() != WSAEWOULDBLOCK))
#else
				if (bytes == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK ||
					errno == EINTR))
#endif // _MSC_VER
				{
					FD_CLR(io_ctx.socket_, &source_send_fds_);
					FD_CLR(io_ctx.socket_, &source_recv_fds_);
					FD_CLR(io_ctx.socket_, &source_except_fds_);
					FD_CLR(io_ctx.socket_, &send_fds_);
					FD_CLR(io_ctx.socket_, &recv_fds_);
					FD_CLR(io_ctx.socket_, &except_fds_);
					socket_closer_(io_ctx.socket_);
					fd_ctx.socket_ = INVALID_SOCKET;
					retired = true;
					return;
				}
				io_ctx.send_bytes_ += bytes;
				if (io_ctx.to_send_ == io_ctx.send_bytes_)
				{
					FD_CLR(io_ctx.socket_, &source_send_fds_);
					FD_CLR(io_ctx.socket_, &source_recv_fds_);
					FD_CLR(io_ctx.socket_, &source_except_fds_);
					FD_CLR(io_ctx.socket_, &send_fds_);
					FD_CLR(io_ctx.socket_, &recv_fds_);
					FD_CLR(io_ctx.socket_, &except_fds_);
					socket_closer_(io_ctx.socket_);
					fd_ctx.socket_ = INVALID_SOCKET;
					retired = true;
					return;
				}
			}
			else if (io_ctx.status_ == io_context::e_accept)
			{
				io_ctx.acceptor_->on_accept(true);
			}
			else if (io_ctx.status_ == io_context::e_connect)
			{
				io_ctx.connector_->on_connect(true);
			}
		}
		void except_callback(fd_context &fd_ctx)
		{
			if (fd_ctx.send_ctx_)
			{
				if (fd_ctx.send_ctx_->status_ == io_context::e_send)
					fd_ctx.send_ctx_->connection_->send_callback(false);

				else if (fd_ctx.send_ctx_->status_ & io_context::e_send &&
					fd_ctx.send_ctx_->status_ & io_context::e_close)
				{
					del_fd_context(fd_ctx);
				}
				else if (fd_ctx.send_ctx_->status_ == io_context::e_accept)
				{
					fd_ctx.send_ctx_->acceptor_->on_accept(false);
				}
				else if (fd_ctx.send_ctx_->status_ == io_context::e_connect)
				{
					fd_ctx.send_ctx_->connector_->on_connect(false);
				}
				else if (fd_ctx.send_ctx_->status_ & io_context::e_close)
				{
					del_fd_context(fd_ctx);
				}
			}
			if (fd_ctx.recv_ctx_)
			{
				if (fd_ctx.recv_ctx_->status_ == io_context::e_recv)
				{
					fd_ctx.recv_ctx_->connection_->recv_callback(false);
				}
				else if (fd_ctx.recv_ctx_->status_ & io_context::e_close)
				{
					del_fd_context(fd_ctx);
				}
				else if (fd_ctx.recv_ctx_->status_ == io_context::e_accept)
				{
					fd_ctx.recv_ctx_->acceptor_->on_accept(false);
				}
				else if (fd_ctx.recv_ctx_->status_ == io_context::e_connect)
				{
					fd_ctx.recv_ctx_->connector_->on_connect(false);
				}
			}
		}
		void del_fd_context(fd_context fd_ctx)
		{
			FD_CLR(fd_ctx.socket_, &source_except_fds_);
			FD_CLR(fd_ctx.socket_, &source_recv_fds_);
			FD_CLR(fd_ctx.socket_, &source_send_fds_);
			FD_CLR(fd_ctx.socket_, &except_fds_);
			FD_CLR(fd_ctx.socket_, &recv_fds_);
			FD_CLR(fd_ctx.socket_, &send_fds_);

			socket_closer()(fd_ctx.socket_);
			if (fd_ctx.recv_ctx_)
				delete fd_ctx.recv_ctx_;
			if (fd_ctx.send_ctx_)
				delete fd_ctx.send_ctx_;
			fd_ctx.send_ctx_ = nullptr;
			fd_ctx.recv_ctx_ = nullptr;
			fd_ctx.socket_ = INVALID_SOCKET;
			retired = true;
			return;
		}
		fd_context_vec fd_ctxs_;

		fd_set source_recv_fds_;
		fd_set source_send_fds_;
		fd_set source_except_fds_;

		fd_set recv_fds_;
		fd_set send_fds_;
		fd_set except_fds_;

		SOCKET maxfd_ = INVALID_SOCKET;

		bool retired = false;

		bool is_stop_ = false;
		socket_closer socket_closer_;
		timer_manager timer_manager_;
		selecter selecter_;
	};
}
typedef select::connection_impl connection_impl;
typedef select::acceptor_impl acceptor_impl;
typedef select::proactor_impl proactor_impl;
typedef select::connector_impl connector_impl;
}
}
