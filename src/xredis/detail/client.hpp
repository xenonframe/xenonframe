#pragma once
namespace xredis
{
	namespace detail
	{
		class client
		{
		public:
			client(xnet::connector && _connector)
				:connector_(std::move(_connector))
			{
				init();
			}
			~client()
			{

			}
			template<typename CB>
			void req(std::string &&req, CB &&cb)
			{
				reply_parser_.regist_callback(std::move(cb));
				if (is_connected_ == false)
				{
					send_buffer_queue_.emplace_back(std::move(req));
					return;
				}
				if (send_buffer_queue_.size())
				{
					send_buffer_queue_.emplace_back(std::move(req));
					if (!is_send_)
					{
						conn_.async_send(std::move(send_buffer_queue_.front()));
						send_buffer_queue_.pop_front();
					}
				}
				else if (!is_send_)
				{
					conn_.async_send(std::move(req));
				}
				else
				{
					send_buffer_queue_.emplace_back(std::move(req));
				}
			}
			void regist_close_callback(std::function<void(client&)> &&func)
			{
				close_callback_ = std::move(func);
			}
			template<typename CB>
			void regist_slots_moved_callback(CB cb)
			{
				slots_moved_callback_ = cb;
			}
			void set_master_info(const master_info &info)
			{
				master_info_ = info;
			}
			master_info &get_master_info()
			{
				return master_info_;
			}
			void regist_connect_success_callback(std::function<void()> &&callback)
			{
				connect_success_callback_ = std::move(callback);
			}
			void regist_connect_failed_callback(std::function<void(const std::string&)> &&callback)
			{
				connect_failed_callback_ = std::move(callback);
			}
			void connect(const std::string &ip, int port)
			{
				ip_ = ip;
				port_ = port;
				connector_.async_connect(ip, port);
			}
		private:
			void init()
			{
				reply_parser_.regist_moved_error_callback(
					[this](const std::string &error) {

				});
				connector_.bind_success_callback(
					std::bind(&client::connect_success_callback,
						this,
						std::placeholders::_1));
				connector_.bind_fail_callback(
					std::bind(&client::connect_failed_callback,
						this,
						std::placeholders::_1));
			}
			void connect_success_callback(xnet::connection &&conn)
			{
				regist_connection(std::move(conn));
				if(connect_success_callback_)
					connect_success_callback_();
			}
			void connect_failed_callback(std::string error_code)
			{
				if(connect_failed_callback_)
					connect_failed_callback_(error_code);
				reply_parser_.close(std::move(error_code));
			}
			void regist_connection(xnet::connection &&conn)
			{
				is_connected_ = true;
				conn_ = std::move(conn);
				conn_.regist_recv_callback([this](char *data, std::size_t len) {
					if (len == 0)
					{
						close_callback();
						conn_.close();
						return;
					}
					reply_parser_.parse((char*)data, len);
					conn_.async_recv_some();
				});
				conn_.regist_send_callback([this](std::size_t  len) {
					if (len == 0)
					{
						conn_.close();
						is_send_ = false;
						close_callback();
						return;
					}
					send_req();
				});
				send_req();
				conn_.async_recv_some();
			}
			void send_req()
			{
				if (send_buffer_queue_.size())
				{
					conn_.async_send(std::move(send_buffer_queue_.front()));
					send_buffer_queue_.pop_front();
					is_send_ = true;
					return;
				}
				is_send_ = false;
			}
			void close_callback()
			{
				reply_parser_.close("lost connection error");
				if (close_callback_)
					close_callback_(*this);
			}
			std::string ip_;
			int port_;
			std::function<void(std::string)> connect_failed_cb_;
			std::function<void(xnet::connection &&)> connect_success_cb_;
			std::function<void(const std::string &, client &)> slots_moved_callback_;
			std::function<void(client &)> close_callback_;
			std::function<void()> connect_success_callback_;
			std::function<void(const std::string&)> connect_failed_callback_;
			std::list<std::string> send_buffer_queue_;
			bool is_send_ = false;
			xnet::connection conn_;
			xnet::connector connector_;
			master_info master_info_;
			reply_parser reply_parser_;
			bool is_connected_ = false;
		};
	}
}