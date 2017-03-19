#pragma once
#include <map>
#include <string>
#include <functional>
#include <iostream>
#include "../xutil/ypipe.hpp"
#include "../xutil/xworker_pool.hpp"
#include "../xnet/xnet.hpp"
#include "../xutil/endec.hpp"
#include "../xutil/gen_endec.hpp"

namespace xactor
{
	


	using engine_id = int64_t;

	struct address
	{
		engine_id engine_id_;
		int64_t actor_id_;
		XENDEC(engine_id_, actor_id_);
	};
	struct address_less 
	{
		bool operator()(const address &lhs, const address &rhs) const
		{
			if (lhs.engine_id_ < rhs.engine_id_)
				return true;
			else if (lhs.actor_id_ < rhs.actor_id_)
				return true;
			return false;
		}
	};

	/*
	| magic_code| msg length| source addr| destination addr | msg type | msg |
	*/

	struct msg
	{
		address source_;
		address dest_;
		std::string type_;
		std::string data_;
		XENDEC(source_, dest_, type_, data_);
	};


	template<typename T>
	inline constexpr char *get_msg_name()
	{
		return T::name;
	}


	template<typename T>
	inline T to_msg(const std::string &data)
	{
		return T();
	}

	

	class actor
	{
	public:
		using ptr_t = std::shared_ptr<actor>;
		using wptr_t = std::weak_ptr<actor>;

		actor()
		{

		}
		virtual ~actor()
		{

		}
	protected:
		template<typename T>
		void regist_msg(const std::function<void(T &&)>& func)
		{
			auto type = get_msg_name<T>();
			auto itr = msg_handles_.find(type);
			if (itr != msg_handles_.end())
				throw std::logic_error(type + " repeated regist");
			msg_handles_.emplace(type, [func](const std::string &datat){
				func(to_msg());
			});
		}
		void regist_timer(const std::function<void()> &func, int64_t millis)
		{

		}
	private:
		friend class xengine;
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
			auto itr = msg_handles_.find(m.type_);
			if (itr == msg_handles_.end())
				throw std::logic_error("can't find " + m.type_+ " msg handle");
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
			int length_ = 0;
		};
		void init()
		{
			conn_.regist_send_callback([this](std::size_t len) {

				if (!len)
				{
					close();
					return;
				}
				send_msg();

			}).regist_recv_callback([this](char *data, std::size_t len) {
				
				uint8_t *ptr = (uint8_t*)data;
				uint8_t *end = ptr + len;

				if (!len)
				{
					close();
					return;
				}
				if (status_ == e_head)
				{
					auto magic_code = xutil::endec::get<int>(ptr, end);
					if (magic_code != header_.magic_num)
					{
						std::cout << "magic_code error " << magic_code << std::endl;
						return close();
					}
					header_.length_ = xutil::endec::get<int>(ptr, end);
					status_ = e_data;
					conn_.async_recv(header_.length_);
				}
				else if(status_ == e_data)
				{
					try
					{
						msg_callback_(std::move(xutil::endec::get<msg>(ptr, end)));
					}
					catch (const std::exception& e)
					{
						std::cout << e.what() << std::endl;
						return close();
					}
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
		void close()
		{
			conn_.close();
			close_callback_();
		}
		enum 
		{
			e_head,
			e_data,
		}status_;

		header header_ = {0, 0};
		std::function<void()> close_callback_;
		std::function<void(msg &&)> msg_callback_;
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

		static constexpr int magic_code = 1213141516;
		struct handshake
		{
			int magic_code_;
			int from_;
			int to_;
			int role;
			XENDEC(magic_code_, from_, to_, role);
		};
		struct endpoint
		{
			
		public:
			endpoint(xnet::connection &&_conn)
				:conn_(std::move(_conn))
			{
				
			}
			void init()
			{
				conn_.regist_recv_callback([this](char *data, std::size_t len) {

					if (!len)
						return close();
					uint8_t *ptr = reinterpret_cast<uint8_t*>(data);
					uint8_t *end = ptr + len;
					try
					{
						handshake h = xutil::endec::get<handshake>(ptr, end);
						if (h.magic_code_ != magic_code)
							throw std::logic_error("magic code error " +
								std::to_string(h.magic_code_));
						if (h.to_ != engine_id_)
							throw std::logic_error("engine_id not match engine_id:" +
								std::to_string(engine_id_) +
								" handshake::to_" +
								std::to_string(h.to_));

						handle_handeshake_(h);
					}
					catch (const std::exception& e)
					{
						std::cout << e.what() << std::endl;
						return close();
					}
				});
				conn_.regist_send_callback([this](auto len) {
					if (!len)
						return close();
				});

				conn_.async_recv(sizeof(handshake));
			}
			void close()
			{
				close_callback_();
				conn_.close();
			}

			std::function<void()> close_callback_;
			std::function<void(const handshake&)> handle_handeshake_;
			xnet::connection conn_;
			address addr_;
			engine_id engine_id_;

			std::list<endpoint>::iterator endpoints_itr_;
		};
		struct endpoints
		{
			std::mutex mutex_;
			std::list<endpoint> endpoints_;

		};
		using endpoints_itr = std::list<endpoint>::iterator;


		struct actors
		{
			std::mutex mutex_;
			std::map<address, actor::ptr_t, address_less> actors_;;
		};

		void init()
		{
			worker_pool_.reset(new xutil::xworker_pool());
		}

		bool handle_msg(msg &&m)
		{
			std::lock_guard<std::mutex> lg(actors_.mutex_);
			auto itr = actors_.actors_.find(m.dest_);
			if (itr == actors_.actors_.end())
				return false;
			if (!itr->second->recv(std::move(m)))
				to_apply_msg(itr->second);
			
		}

		void to_apply_msg(const actor::ptr_t &actor_)
		{
			actor::wptr_t actor_ = actor_;
			auto job = [actor_, this] {
				apply_msg(actor_);
			};
			worker_pool_->add_job(std::move(job));
		}

		void apply_msg(const actor::wptr_t &actor_ptr)
		{
			auto actor_ = actor_ptr.lock();
			if (actor_) 
			{
				if (actor_->apply_once())
					to_apply_msg(actor_);
			}
		}
		void remove_endpoint(const endpoints_itr& itr)
		{
			std::lock_guard<std::mutex> lg(endpoints_.mutex_);
			endpoints_.endpoints_.erase(itr);
		}
		void handle_handshake(endpoints_itr const& itr, const handshake &h)
		{

		}
		void add_endpoint(xnet::connection &&conn)
		{
			endpoint e(std::move(conn));
			endpoints_itr itr;
			do 
			{
				std::lock_guard<std::mutex> lg(endpoints_.mutex_);
				itr = endpoints_.endpoints_.insert(endpoints_.endpoints_.end(), std::move(e));

			} while (0);
			itr->engine_id_ = engine_id_;
			itr->endpoints_itr_ = itr;
			itr->close_callback_ = [itr, this]{
				remove_endpoint(itr);
			};
			itr->handle_handeshake_ = [itr, this](const handshake &h) {
				handle_handshake(itr, h);
			};
			itr->init();
		}

		actors actors_;
		endpoints endpoints_;

		int worker_size_;
		std::unique_ptr<xutil::xworker_pool> worker_pool_;
		xnet::proactor_pool proactor_;

		engine_id engine_id_;
	};
}