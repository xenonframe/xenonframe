namespace xnet
{
	class acceptor : public xutil::no_copy_able
	{
	public:
		typedef std::function<void(connection &&) > accept_callback_t;
		~acceptor()
		{
			close();
		}
		acceptor(acceptor &&acceptor_)
		{
			reset_move(std::move(acceptor_));
		}
		acceptor &operator =(acceptor &&acceptor_)
		{
			reset_move(std::move(acceptor_));
			return *this;
		}
		void regist_accept_callback(accept_callback_t callback)
		{
			accept_callback_ = callback;

		}
		bool bind(const std::string &ip, int port)
		{
			try
			{
				impl_->bind(ip, port);
			}
			catch (std::exception &e)
			{
				std::cout << e.what() << std::endl;
				return false;
			}
			return true;
		}
		bool get_addr(std::string &ip, int &port)
		{
			try
			{
				assert(impl_);
				impl_->get_addr(ip, port);
			}
			catch (std::exception &e)
			{
				std::cout << e.what() << std::endl;
				return false;
			}
			return true;
		}
		void close()
		{
			if (impl_)
			{
				impl_->close();
				impl_ = NULL;
			}
		}
		proactor &get_proactor()
		{
			return *proactor_;
		}
	private:
		acceptor()
		{

		}
		void reset_move(acceptor &&acceptor_)
		{
			if (this == &acceptor_)
				return;
			proactor_ = acceptor_.proactor_;
			init(acceptor_.impl_);
			acceptor_.impl_ = NULL;
			accept_callback_ = std::move(acceptor_.accept_callback_);
		}
		void init(detail::acceptor_impl *impl)
		{
			impl_ = impl;
			impl_->regist_accept_callback(
				[this](detail::connection_impl *conn)
			{
				xnet_assert(conn);
				accept_callback_(std::move(connection(conn)));
			});
		}
		friend proactor;
		proactor *proactor_;
		accept_callback_t accept_callback_;
		detail::acceptor_impl *impl_ = nullptr;
	};
}