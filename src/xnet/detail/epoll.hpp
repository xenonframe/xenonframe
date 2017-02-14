#pragma once
namespace xnet
{
namespace detail
{
namespace epoll
{
	class socket_exception :std::exception
	{
	public:
		socket_exception(int error_code)
			:error_code_(error_code)
		{
			const char *errstr = strerror(error_code_);
			if(errstr)
				error_str_ = errstr;
		}
		socket_exception()
		{

		}
		const char *str()
		{
			return error_str_.c_str();
		}
	private:
		int error_code_ = 0;
		std::string error_str_;
	};
	class io_context
	{
	public:
		io_context(){ }
		~io_context(){ }
		void reload(std::string && data)
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

		void *event_ctx_ = NULL;
		int socket_ = -1;

		std::string buffer_;
		std::size_t to_recv_;
		std::size_t recv_bytes_;

		std::size_t to_send_;
		std::size_t send_bytes_;

		const int recv_some_ = 1024;

		class connection_impl *connection_ = NULL;
		class acceptor_impl *acceptor_ = NULL;
		class connector_impl *connector_ = NULL;
	};

	class connection_impl
	{
	public:
		typedef std::function<void(char*, int)> recv_callback_t;
		typedef std::function<void(int)> send_callback_t;
		connection_impl(SOCKET _socket)
			:socket_(_socket)
		{

		}
		~connection_impl()
		{
		}
		void bind_recv_callback(recv_callback_t callback)
		{
			recv_callback_handle_ = callback;
		}
		void bind_send_callback(send_callback_t callback)
		{
			send_callback_handle_ = callback;
		}
		int send(const char *data, int len)
		{
			return ::send(socket_, data, len, 0);
		}
		void async_send(std::string  &&data)
		{
			xnet_assert(send_ctx_->status_ == io_context::e_idle);
			send_ctx_->reload(std::move(data));
			send_ctx_->status_ = io_context::e_send;
			if(send_ctx_->last_status_ == io_context::e_send)
				return;
			xnet_assert(regist_send_ctx_);
			check_regist();
			regist_send_ctx_(send_ctx_);
		}
		void async_recv(std::size_t len)
		{
			xnet_assert(recv_ctx_->status_ == io_context::e_idle);
			recv_ctx_->reload(len);
			recv_ctx_->status_ = io_context::e_recv;
			if(recv_ctx_->last_status_ == io_context::e_recv)
				return;
			xnet_assert(regist_recv_ctx_);
			check_regist();
			regist_recv_ctx_(recv_ctx_);
		}
		void close()
		{
			if(send_ctx_->status_ == io_context::e_idle)
			{
				unregist_connection_(this);
			}
			else if(send_ctx_->status_ == io_context::e_send)
			{
				send_ctx_->status_ |= io_context::e_close;
			}
			if(in_callback_)
				close_flag_ = true;
			else
			{
				delete this;
			}
		}

	private:
		friend class proactor_impl;
		friend class acceptor_impl;
		friend class connector_impl;
		void check_regist()
		{
			if (!is_regist_)
				regist_connection_(this);
			is_regist_ = true;
		}
		void init()
		{
			send_ctx_ = new io_context;
			if(!recv_ctx_)
				recv_ctx_ = new io_context;
			send_ctx_->connection_ = this;
			send_ctx_->socket_ = socket_;
			recv_ctx_->connection_ = this;
			recv_ctx_->socket_ = socket_;
		}
		void send_callback(bool status)
		{
			send_ctx_->last_status_ = send_ctx_->status_;
			send_ctx_->status_ = io_context::e_idle;
			in_callback_ = true;
			if(status)
				send_callback_handle_(send_ctx_->send_bytes_);
			else
				send_callback_handle_(0);
			in_callback_ = false;
			if(status && 
				!close_flag_  &&
				send_ctx_->status_ != io_context::e_send)
			{
				send_ctx_->last_status_ = io_context::e_idle;
				unregist_send_ctx_(send_ctx_);
			}
			if(close_flag_)
				delete this;
		}
		void recv_callback(bool status)
		{
			recv_ctx_->last_status_ = recv_ctx_->status_;
			recv_ctx_->status_ = io_context::e_idle;
			recv_ctx_->buffer_.push_back('\0');
			in_callback_ = true;
			if(status)
				recv_callback_handle_(
					(char*)recv_ctx_->buffer_.data(),
						recv_ctx_->recv_bytes_);
			else
				recv_callback_handle_(NULL, 0);
			
			in_callback_ = false;
			if(status && 
				!close_flag_ && 
				recv_ctx_->status_ != io_context::e_recv)
			{
				recv_ctx_->last_status_ = io_context::e_idle;
				unregist_recv_ctx_(recv_ctx_);
			}
			if(close_flag_)
				delete this;
		}
		int socket_ = -1;
		bool is_regist_ = false;
		bool close_flag_ = false;
		bool in_callback_ = false;
		io_context *send_ctx_ = NULL;
		io_context *recv_ctx_ = NULL;

		std::function<void(io_context*)> regist_send_ctx_;
		std::function<void(io_context*)> unregist_send_ctx_;
		std::function<void(io_context*)> regist_recv_ctx_;
		std::function<void(io_context*)> unregist_recv_ctx_;
		std::function<void(connection_impl*)> regist_connection_;
		std::function<void(connection_impl*)> unregist_connection_;

		std::function<void(char*, int)> recv_callback_handle_;
		std::function<void(int)> send_callback_handle_;
	};


	class acceptor_impl
	{
	public:
		acceptor_impl()
		{
		}
		~acceptor_impl()
		{

		}
		void regist_accept_callback(
			std::function<void(connection_impl *)> callback)
		{
			acceptor_callback_ = callback;
		}
		void bind(const std::string &ip, int port)
		{
			socket_ = socket(AF_INET, SOCK_STREAM, 0);
			xnet_assert(socket_ != -1);
			xnet_assert(!setnonblocker()(socket_));

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
			inet_aton(ip.c_str(), &(addr.sin_addr));

			xnet_assert(!::bind(socket_,
				(struct sockaddr*)&addr, sizeof(addr)));
			xnet_assert(!listen(socket_, 100));

			accept_ctx_ = new io_context;
			accept_ctx_->acceptor_ = this;
			accept_ctx_->socket_ = socket_;
			accept_ctx_->status_ = io_context::e_accept;

			xnet_assert(regist_accept_ctx_);
			xnet_assert(regist_acceptor_);
			xnet_assert(accept_ctx_); 
 			regist_acceptor_(this);
			regist_accept_ctx_(accept_ctx_);
		}
		void get_addr(std::string &ip, int &port)
		{
			sockaddr_in addr;
			socklen_t len = sizeof(addr);
			xnet_assert(!getsockname(socket_, 
				(struct sockaddr*)&addr, &len));
			ip = inet_ntoa(addr.sin_addr);
			port = ntohs(addr.sin_port);
		}
		void close()
		{
			
			unregist_acceptor_(this);
			
			if(is_callback_)
				is_close_ = true;
			else
				delete this;
		}
	private:
		friend class proactor_impl;

		void on_accept(bool result)
		{
			xnet_assert(result);
			do
			{
				SOCKET sock = ::accept(socket_, NULL, NULL);
				if(sock == -1)
				{
					if(errno == EAGAIN  || errno == EWOULDBLOCK)
						return;
					std::cout <<"file:"<<__FILE__<<" line:"<<__LINE__
						<<" errno:"<<errno <<" error_str: "<<strerror(errno) <<" socket:"<<socket_<< std::endl;
					return;
				}
				xnet_assert(!setnonblocker()(sock));
				auto conn = new connection_impl(sock);
				xnet_assert(conn);
				conn->regist_recv_ctx_ = regist_recv_ctx_;
				conn->unregist_recv_ctx_ = unregist_recv_ctx_;
				conn->regist_send_ctx_ = regist_send_ctx_;
				conn->unregist_send_ctx_ = unregist_send_ctx_;
				conn->regist_connection_= regist_connection_;
				conn->unregist_connection_ = unregist_connection_;
				conn->init();
				is_callback_ = true;
				acceptor_callback_(conn);
				is_callback_ = false;
				if(is_close_)
				{
					delete this;
					return ;
				}

			} while(true);
		}
		bool is_callback_ = false;
		bool is_close_ = false;
		SOCKET socket_ = INVALID_SOCKET;
		io_context* accept_ctx_ = NULL;
		std::function<void(io_context*)> regist_accept_ctx_;
		std::function<void(io_context*)> regist_send_ctx_;
		std::function<void(io_context*)> unregist_send_ctx_;
		std::function<void(io_context*)> regist_recv_ctx_;
		std::function<void(io_context*)> unregist_recv_ctx_;

		std::function<void(acceptor_impl*)> regist_acceptor_;
		std::function<void(acceptor_impl*)> unregist_acceptor_;

		std::function<void(connection_impl*)> regist_connection_;
		std::function<void(connection_impl*)> unregist_connection_;

		std::function<void(connection_impl*)> acceptor_callback_;
	};

	class connector_impl
	{
	public:
		connector_impl()
		{
		}
		~connector_impl()
		{ }
		template<class T>
		void bind_success_callback(T callback)
		{
			success_callback_ = callback;
		}
		template<typename T>
		void bind_failed_callback(T callback)
		{
			failed_callback_ = callback;
		}
		void sync_connect(const std::string &ip, int port)
		{
			xnet_assert(!connect_ctx_);
			socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			xnet_assert(socket_ != -1);
			setnonblocker nonblocker;
			xnet_assert(!nonblocker(socket_));
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(ip.c_str());
			addr.sin_port = htons(port);
			connect_ctx_ = new io_context;
			xnet_assert(connect_ctx_);
			connect_ctx_->connector_ = this;
			connect_ctx_->socket_ = socket_;
			connect_ctx_->status_ = io_context::e_connect;

			int res = connect(socket_, 
				(struct sockaddr*)&addr, sizeof(addr));
			if(res == 0)
			{
				on_connect(true);
				return;
			}
			xnet_assert(errno == EINTR || errno == EINPROGRESS);
			xnet_assert(regist_connector_);
			xnet_assert(regist_connect_ctx_);
			regist_connector_(this);
			regist_connect_ctx_(connect_ctx_);
		}
		void close()
		{
			if(connect_ctx_)
			{
				unregist_connector_(this);
				connect_ctx_ = NULL;
			}
			if(!in_callback_)
				delete this;
			else
				close_flag_ = true;
		}
	private:
		friend class proactor_impl;

		void on_connect(bool result)
		{
			if(!result)
			{
				unregist_connector_(this);
				connect_ctx_ = NULL;
				in_callback_ = true;
				failed_callback_(strerror(errno));
				in_callback_ = false;
				if(close_flag_)
					delete this;
				return;
			}
			xnet_assert(success_callback_);
			xnet_assert(failed_callback_);

			int err = 0;
			socklen_t len = sizeof(err);
			if (getsockopt(socket_,SOL_SOCKET,
				SO_ERROR,(char*)&err,&len) < 0)
			{
				if (errno == EINTR || errno == EINPROGRESS)
					return;
				xnet_assert(false);
			}

			if(err == 0)
			{
				unregist_connect_ctx_(connect_ctx_);
				connect_ctx_->status_ = io_context::e_idle;

				auto conn = new connection_impl(socket_);
				xnet_assert(connect_ctx_);
				conn->recv_ctx_ = connect_ctx_;

				conn->regist_send_ctx_ = regist_send_ctx_;
				conn->unregist_send_ctx_ = unregist_send_ctx_;
				conn->regist_recv_ctx_ = regist_recv_ctx_;
				conn->unregist_recv_ctx_ = unregist_recv_ctx_;
				conn->regist_connection_ = regist_connection_;
				conn->unregist_connection_ = unregist_connection_;
				conn->init();

				socket_ = INVALID_SOCKET;
				connect_ctx_->connector_ = nullptr;
				connect_ctx_ = NULL;
				in_callback_ = true;
				success_callback_(conn);
				in_callback_ = false;
				if(close_flag_)
					delete this;
			}
			else
			{
				in_callback_ = true;
				failed_callback_("connect failed");
				in_callback_ = false;
				if(close_flag_)
					delete this;
			}
		}
		bool in_callback_ = false;
		bool close_flag_ = false;
		SOCKET socket_ = INVALID_SOCKET;
		io_context *connect_ctx_ = NULL;

		std::function<void(io_context*)> regist_send_ctx_;
		std::function<void(io_context*)> unregist_send_ctx_;
		std::function<void(io_context*)> regist_recv_ctx_;
		std::function<void(io_context*)> unregist_recv_ctx_;
		
		std::function<void(io_context*)> regist_connect_ctx_;
		std::function<void(io_context*)> unregist_connect_ctx_;

		std::function<void(connection_impl*)> regist_connection_;
		std::function<void(connection_impl*)> unregist_connection_;

		std::function<void(connector_impl *)> regist_connector_;
		std::function<void(connector_impl *)> unregist_connector_;

		std::function<void(connection_impl*)> success_callback_;
		std::function<void(std::string)> failed_callback_;
	};

	class proactor_impl
	{
	private:
		struct event_context
		{
			int socket_ = -1;
			epoll_event ev_;
			io_context *send_ctx_ = NULL;
			io_context *recv_ctx_ = NULL;
		};
	public:
		proactor_impl()
		{

		}
		void run()
		{
			while(is_stop_ == false)
			{
				auto timeout = timer_manager_.do_timer();
				if(timeout == 0) timeout = 1000;
				
				int n = epoll_wait(epfd_, events_, 
					MAXIOEVENTS, timeout);
				if(n == -1)
				{
					xnet_assert(errno == EINTR);
					continue;
				}
				else if(n == 0) 
					continue;
				try
				{
					for (int i = 0; i < n; i++)
					{
						event_context *event_ctx =
							(event_context*)events_[i].data.ptr;
						xnet_assert(event_ctx);
						if (event_ctx->socket_ == -1)
							continue;
						if (events_[i].events & (EPOLLERR | EPOLLHUP))
							except_callback(event_ctx);
						if (event_ctx->socket_ == -1)
							continue;
						if (events_[i].events & EPOLLIN)
							readable_callback(event_ctx);
						if (event_ctx->socket_ == -1)
							continue;
						if (events_[i].events & EPOLLOUT)
							writeable_callback(event_ctx);
					}
				}
				catch (const std::exception& e)
				{
					std::cout << e << std::endl;
				}
				
				if(del_event_ctx_.size())
				{
					for (auto itr: del_event_ctx_)
					{
						if(itr->recv_ctx_)
							delete itr->recv_ctx_;
						if(itr->send_ctx_)
							delete itr->send_ctx_;
						delete itr;
					}
					del_event_ctx_.clear();
				}
			}
		}
		void stop()
		{
			is_stop_ = true;
		}
		connector_impl *get_connector()
		{
			connector_impl *connector = new connector_impl;

			connector->regist_send_ctx_ =
				std::bind(&proactor_impl::regist_send, this,
							std::placeholders::_1);
			connector->unregist_send_ctx_ =
				std::bind(&proactor_impl::unregist_send, this,
							std::placeholders::_1);
			connector->regist_recv_ctx_ =
				std::bind(&proactor_impl::regist_recv, this,
							std::placeholders::_1);
			connector->unregist_recv_ctx_ =
				std::bind(&proactor_impl::unregist_recv, this,
							std::placeholders::_1);
			connector->regist_connector_=
				std::bind(&proactor_impl::regist_connector, this,
					std::placeholders::_1);
			connector->unregist_connector_=
				std::bind(&proactor_impl::unregist_connector, this,
					std::placeholders::_1);
			connector->regist_connection_ =
				std::bind(&proactor_impl::regist_connection2, this,
							std::placeholders::_1);
			connector->unregist_connection_ =
				std::bind(&proactor_impl::unregist_connection, this,
						  std::placeholders::_1);
			connector->regist_connect_ctx_ =
				std::bind(&proactor_impl::regist_send, this,
							std::placeholders::_1);
			connector->unregist_connect_ctx_ = 
				std::bind(&proactor_impl::unregist_send, this,
					std::placeholders::_1);

			return connector;
		}
		acceptor_impl *get_acceptor()
		{
			acceptor_impl *acceptor = new acceptor_impl;
			acceptor->regist_send_ctx_ =
				std::bind(&proactor_impl::regist_send, this,
						  std::placeholders::_1);
			acceptor->unregist_send_ctx_ =
				std::bind(&proactor_impl::unregist_send, this,
						  std::placeholders::_1);
			acceptor->regist_recv_ctx_ =
				std::bind(&proactor_impl::regist_recv, this,
						  std::placeholders::_1);
			acceptor->unregist_recv_ctx_ =
				std::bind(&proactor_impl::unregist_recv, this,
						  std::placeholders::_1);
			acceptor->regist_acceptor_ = 
				std::bind(&proactor_impl::regist_acceptor, this,
						  std::placeholders::_1);
			acceptor->unregist_acceptor_ =
				std::bind(&proactor_impl::unregist_acceptor, this,
					std::placeholders::_1);
			acceptor->regist_connection_ =
				std::bind(&proactor_impl::regist_connection2, this,
						  std::placeholders::_1);
			acceptor->unregist_connection_ =
				std::bind(&proactor_impl::unregist_connection, this,
						  std::placeholders::_1);
			acceptor->regist_accept_ctx_ =
				std::bind(&proactor_impl::regist_recv, this,
						  std::placeholders::_1);

			return acceptor;
		}
		template<typename T>
		timer_id set_timer(std::size_t timeout, T &&timer_callback)
		{
			return timer_manager_.set_timer(timeout, std::forward<T>(timer_callback));
		}
		void cancel_timer(timer_manager::timer_id id)
		{
			timer_manager_.cancel_timer(id);
		}
		void init()
		{
			epfd_ = epoll_create(MAXIOEVENTS);
		}
		void regist_connection(connection_impl &conn)
		{
			conn.regist_send_ctx_ =
				std::bind(&proactor_impl::regist_send, this,
					std::placeholders::_1);
			conn.unregist_send_ctx_ =
				std::bind(&proactor_impl::unregist_send, this,
					std::placeholders::_1);
			conn.regist_recv_ctx_ =
				std::bind(&proactor_impl::regist_recv, this,
					std::placeholders::_1);
			conn.unregist_recv_ctx_ =
				std::bind(&proactor_impl::unregist_recv, this,
					std::placeholders::_1);
			conn.regist_connection_ =
				std::bind(&proactor_impl::regist_connection2, this,
					std::placeholders::_1);
			conn.unregist_connection_ =
				std::bind(&proactor_impl::unregist_connection, this,
					std::placeholders::_1);
		}
	private:
		void regist_connection2(connection_impl *conn)
		{
			event_context *event_ctx = nullptr;
			if (conn->recv_ctx_->event_ctx_)
				event_ctx =reinterpret_cast<event_context *>(
						conn->recv_ctx_->event_ctx_);
			else
				event_ctx = new event_context;
			xnet_assert(event_ctx);
			memset(event_ctx, 0, sizeof(event_context));
			conn->send_ctx_->event_ctx_ = event_ctx;
			conn->recv_ctx_->event_ctx_ = event_ctx;
			event_ctx->recv_ctx_ = conn->recv_ctx_;
			event_ctx->send_ctx_ = conn->send_ctx_;
			event_ctx->socket_ = conn->socket_;
			event_ctx->ev_.events = 0;
			event_ctx->ev_.data.ptr = event_ctx;
			auto ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, 
					event_ctx->socket_, &event_ctx->ev_);
			xnet_assert(!ret || errno == EEXIST);
		}
		void unregist_connection(connection_impl *conn)
		{
			unregist_event_context(
				(event_context*)conn->recv_ctx_->event_ctx_);
		}
		void regist_acceptor(acceptor_impl *acceptor)
		{
			event_context *event_ctx = new event_context;
			memset(event_ctx, 0, sizeof(event_context));
			acceptor->accept_ctx_->event_ctx_ = event_ctx;
			event_ctx->recv_ctx_ = acceptor->accept_ctx_;
			event_ctx->socket_ = acceptor->socket_;
			event_ctx->ev_.events = 0;
			event_ctx->ev_.data.ptr = event_ctx;
			xnet_assert(!epoll_ctl(epfd_, EPOLL_CTL_ADD,
				event_ctx->socket_, &event_ctx->ev_));
		}
		void unregist_acceptor(acceptor_impl *acceptor)
		{
			unregist_event_context(
				(event_context*)acceptor->accept_ctx_->event_ctx_);
		}

		void regist_connector(connector_impl *connector)
		{
			event_context *event_ctx = new event_context;
			memset(event_ctx, 0, sizeof(event_context));
			connector->connect_ctx_->event_ctx_ = event_ctx;
			event_ctx->send_ctx_ = connector->connect_ctx_;
			event_ctx->socket_ = connector->socket_;
			event_ctx->ev_.events = 0;
			event_ctx->ev_.data.ptr = event_ctx;

			xnet_assert(!epoll_ctl(epfd_, EPOLL_CTL_ADD,
						event_ctx->socket_, &event_ctx->ev_));
			
		}
		void unregist_connector(connector_impl *connector)
		{
			unregist_event_context(
				(event_context*)connector->connect_ctx_->event_ctx_);
		}

		void regist_send(io_context *io_ctx)
		{
			xnet_assert(io_ctx);
			event_context *event_ctx = 
				(event_context*)io_ctx->event_ctx_;

			xnet_assert(event_ctx);
			event_ctx->ev_.events |= EPOLLOUT;
			xnet_assert(!epoll_ctl(epfd_, EPOLL_CTL_MOD,
								event_ctx->socket_, &event_ctx->ev_));
		}
		void unregist_send(io_context *io_ctx)
		{
			xnet_assert(io_ctx);
			xnet_assert(io_ctx->event_ctx_);
			event_context *event_ctx = 
				(event_context*)io_ctx->event_ctx_;

			xnet_assert(event_ctx);
			event_ctx->ev_.events &= ~((short)EPOLLOUT);
			xnet_assert(!epoll_ctl(epfd_, EPOLL_CTL_MOD,
						event_ctx->socket_, &event_ctx->ev_));
		}
		void regist_recv(io_context *io_ctx)
		{
			xnet_assert(io_ctx);
			xnet_assert(io_ctx->event_ctx_);
			event_context *event_ctx = 
				(event_context*)io_ctx->event_ctx_;
			xnet_assert(event_ctx);
			event_ctx->ev_.events |= EPOLLIN;
			xnet_assert(!epoll_ctl(epfd_, EPOLL_CTL_MOD,
						event_ctx->socket_, &event_ctx->ev_));
		}
		void unregist_recv(io_context *io_ctx)
		{
			xnet_assert(io_ctx);
			xnet_assert(io_ctx->event_ctx_);
			event_context *event_ctx = 
				(event_context*)io_ctx->event_ctx_;
			xnet_assert(event_ctx);
			event_ctx->ev_.events &= ~((short)EPOLLIN);
			xnet_assert(!epoll_ctl(epfd_, EPOLL_CTL_MOD,
						event_ctx->socket_, &event_ctx->ev_));
		}
		void unregist_event_context(event_context* event_ctx)
		{
			del_event_ctx_.push_back(event_ctx);
			xnet_assert(!epoll_ctl(epfd_, EPOLL_CTL_DEL,
						event_ctx->socket_, &event_ctx->ev_));
			close(event_ctx->socket_);
			event_ctx->socket_ = -1;
		}
		void except_callback(event_context *_event_ctx)
		{
			xnet_assert(_event_ctx);
			event_context &event_ctx = *_event_ctx;
			if(event_ctx.send_ctx_)
			{
				if(event_ctx.send_ctx_->status_ == io_context::e_send)
				{
					event_ctx.send_ctx_->connection_->send_callback(false);
				}
				else if(event_ctx.send_ctx_->status_ & io_context::e_send &&
						event_ctx.send_ctx_->status_ & io_context::e_close)
				{
					unregist_event_context(_event_ctx);
				}
				else if(event_ctx.send_ctx_->status_ == io_context::e_accept)
				{
					event_ctx.send_ctx_->acceptor_->on_accept(false);
				}
				else if(event_ctx.send_ctx_->status_ == io_context::e_connect)
				{
					event_ctx.send_ctx_->connector_->on_connect(false);
				}
			}
			if(event_ctx.recv_ctx_)
			{
				if(event_ctx.recv_ctx_->status_ == io_context::e_recv)
				{
					event_ctx.recv_ctx_->connection_->recv_callback(false);
				}
				else if(event_ctx.recv_ctx_->status_ == io_context::e_accept)
				{
					event_ctx.recv_ctx_->acceptor_->on_accept(false);
				}
				else if(event_ctx.recv_ctx_->status_ == io_context::e_connect)
				{
					event_ctx.recv_ctx_->connector_->on_connect(false);
				}
			}
		}
		void readable_callback(event_context *event_ctx)
		{
			xnet_assert(event_ctx);
			xnet_assert(event_ctx->recv_ctx_);

			io_context &io_ctx = *(event_ctx->recv_ctx_);
			if(io_ctx.status_ == io_context::e_recv)
			{
				std::size_t len = io_ctx.to_recv_ ? 
					io_ctx.to_recv_ : io_ctx.buffer_.size();

				auto bytes = ::recv(io_ctx.socket_,
					(char*)io_ctx.buffer_.data() + io_ctx.recv_bytes_,
					(int)(len - io_ctx.recv_bytes_), 0);
				if(bytes == 0)
				{
					io_ctx.connection_->recv_callback(false);
					return;
				}
				else if (bytes == -1)
				{
					xnet_assert(errno != EBADF
						&& errno != EFAULT
						&& errno != EINVAL
						&& errno != ENOMEM
						&& errno != ENOTSOCK);
					if (errno == EWOULDBLOCK || errno == EINTR)
						return;
				}
				io_ctx.recv_bytes_ += bytes;
				if(io_ctx.to_recv_ <= io_ctx.recv_bytes_)
				{
					io_ctx.connection_->recv_callback(true);
				}
			}
			else if(io_ctx.status_ == io_context::e_accept)
			{
				io_ctx.acceptor_->on_accept(true);
			}
			else if(io_ctx.status_ == io_context::e_connect)
			{
				io_ctx.connector_->on_connect(true);
			}
		}

		void writeable_callback(event_context *event_ctx)
		{
			xnet_assert(event_ctx->send_ctx_);
			io_context &io_ctx = *event_ctx->send_ctx_;

			if(io_ctx.status_ == io_context::e_send)
			{
				auto bytes = ::send(io_ctx.socket_,
					(char*)io_ctx.buffer_.data() + io_ctx.send_bytes_,
					io_ctx.to_send_ - io_ctx.send_bytes_, 0);

				if(bytes == -1 && !(errno == EAGAIN ||
					errno == EWOULDBLOCK ||
					errno == EINTR))
				{
					io_ctx.connection_->send_callback(false);
					return;
				}
				io_ctx.send_bytes_ += bytes;
				if(io_ctx.to_send_ == io_ctx.send_bytes_)
				{
					io_ctx.connection_->send_callback(true);
				}
			}
			else if(io_ctx.status_ & io_context::e_send &&
					io_ctx.status_ & io_context::e_close)
			{
				auto bytes = ::send(io_ctx.socket_,
					(char*)io_ctx.buffer_.data() + io_ctx.send_bytes_,
					io_ctx.to_send_ - io_ctx.send_bytes_, 0);
				if(bytes <= 0)
				{
					unregist_event_context(event_ctx);
					return;
				}
				io_ctx.send_bytes_ += bytes;
				if(io_ctx.to_send_ == io_ctx.send_bytes_)
				{
					unregist_event_context(event_ctx);
					return;
				}
			}
			else if(io_ctx.status_ == io_context::e_accept)
			{
				io_ctx.acceptor_->on_accept(true);
			}
			else if(io_ctx.status_ == io_context::e_connect)
			{
				io_ctx.connector_->on_connect(true);
			}
		}
		timer_manager timer_manager_;
		std::vector<event_context*> del_event_ctx_;
		epoll_event ev_;
		epoll_event events_[MAXIOEVENTS];
		int epfd_ = -1;
		bool is_stop_ = false;
	};
}
typedef epoll::connection_impl connection_impl;
typedef epoll::acceptor_impl acceptor_impl;
typedef epoll::proactor_impl proactor_impl;
typedef epoll::connector_impl connector_impl;
}
}
