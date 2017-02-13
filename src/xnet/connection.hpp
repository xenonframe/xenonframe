namespace xnet
{
	class proactor;
	class connection :public xutil::no_copy_able
	{
	public:
		typedef std::function<void(char*, std::size_t)> recv_callback_t;
		typedef std::function<void(std::size_t)> send_callback_t;

		connection()
		{

		}
		connection(detail::connection_impl *impl)
			:impl_(impl)
		{
			xnet_assert(impl);
			init();
		}
		connection(connection &&_connection)
		{
			if(_connection.impl_)
				reset_move(std::move(_connection));
		}
		connection &operator=(connection &&_connection)
		{
			if (_connection.impl_)
				reset_move(std::move(_connection));
			return *this;
		}
		~connection()
		{
			close();
		}
		connection &regist_send_callback(send_callback_t callback)
		{
			send_callback_ = callback;
			return *this;
		}

		connection &regist_recv_callback(recv_callback_t callback)
		{
			recv_callback_ = callback;
			return *this;
		}
		int send(const char *data, int len)
		{
			xnet_assert(impl_);
			return impl_->send(data, len);
		}
		void async_send(std::string &&buffer)
		{
			xnet_assert(impl_);
			xnet_assert(buffer.size());
			impl_->async_send(std::move(buffer));
		}

		void async_send(const char *data, uint32_t len)
		{
			xnet_assert(len);
			xnet_assert(data);
			xnet_assert(impl_);
			impl_->async_send({ data, len});
		}

		void async_recv(std::size_t len)
		{
			xnet_assert(len);
			xnet_assert(impl_);
			impl_->async_recv(len);
		}
		void async_recv_some()
		{
			xnet_assert(impl_);
			impl_->async_recv(0);
		}
		void close()
		{
			if (impl_)
			{
				impl_->close();
				impl_ = NULL;
			}
		}
		bool valid()
		{
			return !!impl_;
		}
	private:
		friend proactor;
		void reset_move(connection && _conn)
		{
			if (this != &_conn)
			{
				impl_ = _conn.impl_;
				recv_callback_ = std::move(_conn.recv_callback_);
				send_callback_ = std::move(_conn.send_callback_);
				init();
				_conn.impl_ = NULL;
			}
		}
		void init()
		{
			impl_->bind_recv_callback(
				[this](char *data, std::size_t len) {
				xnet_assert(recv_callback_);
				recv_callback_(data, len);
			});
			impl_->bind_send_callback(
				[this](std::size_t len) {
				xnet_assert(send_callback_);
				send_callback_(len);
			});
		}
		std::size_t send_error_timer_id_ = 0;
		std::size_t recv_error_timer_id_ = 0;
		detail::connection_impl *impl_ = NULL;
		send_callback_t send_callback_;
		recv_callback_t  recv_callback_;

	};
}