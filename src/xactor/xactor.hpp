#pragma once
#include <map>
#include <string>
#include <functional>
#include <iostream>
#include "../xutil/ypipe.hpp"
#include "../xutil/xworker_pool.hpp"
#include "../xnet/xnet.hpp"

namespace xactor
{
	struct msg
	{
		std::string name_;
		std::string data_;
	};
	template<typename MSG>
	inline std::string get_msg_name()
	{
		return MSG::name;
	}
	template<typename MSG>
	inline MSG to_msg(const std::string &data)
	{
		return MSG();
	}

	struct address
	{
		int64_t actor_id_;
	};

	class actor
	{
	public:
		actor()
		{

		}
		virtual ~actor()
		{

		}
	protected:
		template<typename MSG>
		void regist_msg(const std::function<void(MSG &&)>& func)
		{
			auto name = get_msg_name<MSG>();
			auto itr = msg_handles_.find(name);
			if (itr != msg_handles_.end())
				throw std::logic_error(name + " repeated regist");
			msg_handles_.emplace(name, [func](const std::string &datat){
				func(to_msg());
			});
		}
		void regist_timer(const std::function<void()> &func, int64_t millis)
		{

		}
	private:
		bool recv(msg &&m)
		{
			msg_queue_.write(std::move(m));
			return msg_queue_.flush();
		}
		bool apply_all()
		{
			while (apply_once());
		}
		bool apply_once()
		{
			auto m = msg_queue_.read();
			if (!m.first)
				return false;
			apply(m.second);
		}
		void apply(const msg &m)
		{
			auto itr = msg_handles_.find(m.name_);
			if (itr == msg_handles_.end())
				throw std::logic_error("can't find " + m.name_+ "msg handle");
			try
			{
				itr->second(m.data_);
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
		}
		using msg_handle_t = std::function<void(const std::string &)>;
		std::map<std::string, msg_handle_t> msg_handles_;
		address address_;
		xutil::ypipe<msg> msg_queue_;
	};

	class session 
		:public std::enable_shared_from_this<session>
	{
	public:
		using msgbox_t = xnet::proactor_pool::msgbox_t;
		session(xnet::connection &&conn, msgbox_t &msgbox_)
			:msgbox_(msgbox_),
			conn_(std::move(conn))
		{
		}
		void send_msg(std::string &&msg)
		{
			std::lock_guard<std::mutex> lg(msg_queue_lock_);
			msg_queue_.write(std::move(msg));
			if (msg_queue_.flush())
				return;
			regist_send();
		}
	private:
		struct header
		{
			int magic_num = 13141516;
			int length_;
		};
		void init()
		{
			conn_.regist_send_callback([this](std::size_t len) {

				if (!len)
				{
					close_callback_();
					conn_.close();
					return;
				}
				send_msg();

			}).regist_recv_callback([this](char * data, std::size_t len) {
				
				if (!len)
				{
					close_callback_();
					conn_.close();
					return;
				}
				if (status_ == e_head)
				{

				}

			}).async_recv(sizeof(header));
			status_ = e_head;
		}
		void send_msg()
		{
			auto item = msg_queue_.read();
			if (!item.first)
				return;
			conn_.async_send(std::move(item.second));

		}
		void regist_send()
		{
			auto self = shared_from_this();
			msgbox_.send([self,this] {
				send_msg();
			});
		}
		enum 
		{
			e_head,
			e_data_,
		}status_;

		std::function<void()> close_callback_;
		msgbox_t &msgbox_;
		std::mutex msg_queue_lock_;
		xutil::ypipe<std::string> msg_queue_;
		xnet::connection conn_;
	};

	class xengine
	{
	public:
		xengine()
		{

		}
		void regist_actor()
		{

		}
		void bind(const std::string &ip, int port)
		{
			proactor_.regist_accept_callback([](xnet::connection &&conn){

			});
			proactor_.bind(ip, port);
		}
		int workers()
		{
			return worker_size_;
		}
		void set_workers(int size)
		{
			worker_size_ = size;
		}
	private:
		void init()
		{
			worker_pool_.reset(new xutil::xworker_pool());
		}


		int worker_size_;
		std::unique_ptr<xutil::xworker_pool> worker_pool_;
		xnet::proactor_pool proactor_;
	};
}