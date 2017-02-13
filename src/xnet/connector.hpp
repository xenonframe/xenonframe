#pragma once
namespace xnet
{
	class connector : xutil::no_copy_able
	{
	public:
		typedef std::function<void(connection &&)> success_callback_t;
		typedef std::function<void(std::string)> failed_callback_t;

		connector(connector && _connector)
		{
			reset_move(std::move(_connector));
		}
		connector & operator =(connector && _connector)
		{
			reset_move(std::move(_connector));
			return *this;
		}
		~connector()
		{
			close();
		}
		bool async_connect(const std::string &ip, int port)
		{
			ip_ = ip;
			port_ = port;
			try
			{
				impl_->sync_connect(ip_, port_);
			}
			catch (std::exception &e)
			{
				last_error_str_ = e.what();
				std::cout << last_error_str_ << std::endl;
				return false;
			}
			return true;
		}
		connector& bind_success_callback(success_callback_t callback)
		{
			success_callback_ = callback;
			impl_->bind_success_callback(
				[this](detail::connection_impl *conn)
			{
				success_callback_(connection(conn));
			});
			return *this;
		}
		connector& bind_fail_callback(failed_callback_t callback)
		{
			failed_callback_ = callback;
			xnet_assert(impl_);
			impl_->bind_failed_callback(
				[this](std::string error_code)
			{
				failed_callback_(std::move(error_code));
			});
			return *this;
		}
		void close()
		{
			if (impl_)
			{
				impl_->close();
				impl_ = nullptr;
			}
		}
		std::string get_last_error()
		{
			return last_error_str_;
		}
	private:
		friend proactor;

		void init(detail::connector_impl *impl)
		{
			impl_ = impl;
		}
		void reset_move(connector &&_connector)
		{
			if (this == &_connector)
				return;
			xnet_assert(_connector.impl_);
			impl_ = _connector.impl_;
			proactor_ = _connector.proactor_;
			success_callback_ = std::move(_connector.success_callback_);
			failed_callback_ = std::move(_connector.failed_callback_);
			impl_->bind_success_callback(
				[this](detail::connection_impl *conn) {
				success_callback_(connection(conn));
			});
			impl_->bind_failed_callback(
				[this](std::string error_code) {
				failed_callback_(error_code);
			});
			_connector.impl_ = nullptr;
		}
		connector()
		{
		}
		std::string last_error_str_;
		std::function<void(connection &&)> success_callback_;
		failed_callback_t failed_callback_;
		detail::connector_impl *impl_ = nullptr;
		std::string ip_;
		proactor *proactor_ = nullptr;
		int port_;

	};
}