#pragma once
#include <mutex>
namespace xnet
{
	template<typename T>
	class msgbox :xutil::no_copy_able
	{
	public:
		typedef std::function<void()> notify_callback_t;
		msgbox(proactor &pro)
			:proactor_(pro),
			acceptor_(std::move(pro.get_acceptor())),
			connector_(std::move(pro.get_connector()))
		{
			init();
		}
		~msgbox()
		{

		}
		void regist_notify(notify_callback_t handle)
		{
			callback_handle_ = handle;
		}
		void regist_inited_callback(notify_callback_t handle)
		{
			inited_callback_handle_ = handle;
		}
		void send(T &&_msg)
		{
			std::lock_guard<std::mutex> locker(mtx_);
			ypipe_->write(std::forward<T>(_msg));
			if (!ypipe_->flush())
				notify();
		}
		std::pair<bool, T> recv()
		{
			auto item(ypipe_->read());
			if (item.first)
				return std::move(item);
			return{ false, T() };
		}
	private:
		void init()
		{
			ypipe_.reset(new xutil::ypipe<T, 128>);
			xnet_assert(acceptor_.bind("127.0.0.1", 0));
			xnet_assert(acceptor_.get_addr(ip_, port_));
			xnet_assert(!recv().first);
			acceptor_.regist_accept_callback([&](connection &&conn) {
				signal_receiver_ = std::move(conn);
				signal_receiver_.regist_recv_callback([&](char *, std::size_t len){
					if (len == 0)
					{
						xnet_assert(false);
					}
					signal_callback();
					signal_receiver_.async_recv_some();
				});
				signal_receiver_.async_recv_some();
				acceptor_.close();
				init_callback();
			});
			connector_.bind_success_callback([&](connection &&connn) {
				signal_sender_ = std::move(connn);
				connector_.close();
				init_callback();
			});
			connector_.bind_fail_callback([](std::string &&str) {
				std::cout << str << std::endl;
			});
			connector_.async_connect(ip_, port_);
		}
		void signal_callback()
		{
			xnet_assert(callback_handle_);
			callback_handle_();
		}
		void init_callback()
		{
			if (!inited_callback_handle_)
				return;
			if (signal_receiver_.valid() && signal_sender_.valid())
				inited_callback_handle_();
		}
		void notify()
		{
			char buf = 'a';
			signal_sender_.send(&buf, sizeof(buf));
		}
		std::string ip_;
		int port_;
		proactor &proactor_;
		acceptor acceptor_;
		connector connector_;
		connection signal_sender_;
		connection signal_receiver_;

		notify_callback_t callback_handle_;
		notify_callback_t inited_callback_handle_;
		std::mutex mtx_;
		std::unique_ptr<xutil::ypipe<T, 128>> ypipe_;
	};
}
