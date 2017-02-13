#pragma once
namespace xwebsocket
{
	class xserver
	{
	public:
		using accept_callback = std::function<void(session &&)>;

		xserver(std::size_t io_threads = std::thread::hardware_concurrency())
			:proactor_pool_(io_threads)
		{
			
		}
		xserver &bind(const std::string &ip, int port)
		{
			proactor_pool_.bind(ip, port);
			return *this;
		}
		xserver &regist_accept_callback(const  accept_callback&callback)
		{
			callback_ = callback;
			return *this;
		}
		void start()
		{
			proactor_pool_.start();
			proactor_pool_.regist_accept_callback(
				std::bind(&xserver::connection_accept_callback, this, std::placeholders::_1));
		}
		void stop()
		{
			proactor_pool_.stop();
		}
		
	private:
		void connection_accept_callback(xnet::connection &&conn)
		{
			session sess(std::move(conn));
			sess.init();
			callback_(std::move(sess));
		}

		accept_callback callback_;
		xnet::proactor_pool proactor_pool_;
	};
}