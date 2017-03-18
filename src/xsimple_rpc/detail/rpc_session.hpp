#pragma once
namespace xsimple_rpc
{
	namespace detail
	{
		struct rpc_session
		{
			void push_rpc_req(std::shared_ptr<rpc_req> & rpc_req)
			{
				std::lock_guard<std::mutex> locker(mtx_);
				req_item_list_.push_back(rpc_req);
			}

			void do_send_rpc()
			{
				std::lock_guard<std::mutex> locker(mtx_);
				if (is_send_ || req_item_list_.empty())
					return;
				is_send_ = true;
				auto item = req_item_list_.front();
				conn_.async_send(std::move(item->req_buffer_));
				wait_rpc_resp_list_.emplace_back(std::move(item));
				req_item_list_.pop_front();
			}

			void init_conn()
			{
				conn_.regist_send_callback([this](std::size_t len) {

					if (len == 0)
					{
						close();
						return;
					}
					is_send_ = false;
					do_send_rpc();
				});
				conn_.regist_recv_callback([this](char *data, std::size_t len) {

					if (len == 0 || recv_data_callback(data, len) == false)
					{
						close();
						return;
					}
				});
				conn_.async_recv(sizeof(uint32_t));
			}
			bool recv_data_callback(char *data, std::size_t len)
			{
				uint8_t *ptr = reinterpret_cast<uint8_t*>(data);
				uint8_t *end = reinterpret_cast<uint8_t*>(data) + len;
				if (msg_len_ == 0 && len == sizeof(msg_len_))
				{
					msg_len_ = xutil::endec::get<uint32_t>(ptr, end);
					if (msg_len_ < min_rpc_msg_len())
					{
						std::cout << "msg len invalid" << std::endl;
						return false;
					}
					conn_.async_recv((std::size_t)msg_len_ - sizeof(int32_t));
					msg_len_ = 0;
					return true;
				}
				try
				{
					if(magic_code != xutil::endec::get<std::string>(ptr, end))
						return false;

					auto item = wait_rpc_resp_list_.front();
					wait_rpc_resp_list_.pop_front();

					auto req_id = xutil::endec::get<int64_t>(ptr, end);
					if (item->req_id_ != req_id)
					{
						std::cout << "req_id != item->req_id_" << std::endl;
						return false;
					}
					std::unique_lock<std::mutex> locker(mtx_);
					item->result_ = xutil::endec::get<std::string>(ptr, end);
					item->status_ = rpc_req::status::e_ok;
					cv_.notify_one();

				}
				catch (const std::exception& e)
				{
					std::cout << e.what() << std::endl;
					return false;
				}
				conn_.async_recv(sizeof(uint32_t));
				return true;
			}
			void close()
			{
				std::unique_lock<std::mutex> locker(mtx_);
				for (auto &itr : wait_rpc_resp_list_)
				{
					itr->status_ = rpc_req::status::e_rpc_error;
					itr->result_ = "lost connection";
				}
				for (auto &itr : req_item_list_)
				{
					itr->status_ = rpc_req::status::e_rpc_error;
					itr->result_ = "lost connection";
				}
					
				cv_.notify_all();
				is_close_ = true;
				locker.unlock();
				close_callback_();
			}
			uint32_t msg_len_ = 0;
			bool is_close_ = false;
			bool is_send_ = false;
			std::string last_error_code_;
			std::mutex mtx_;
			std::condition_variable cv_;
			xnet::connection conn_;
			std::size_t msgbox_index_ = 0;
			std::function<void()> close_callback_;
			std::list<std::shared_ptr<rpc_req>> req_item_list_;
			std::list<std::shared_ptr<rpc_req>> wait_rpc_resp_list_;
		};
	}
	
}
