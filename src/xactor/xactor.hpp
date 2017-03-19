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
	using engine_id = uint64_t;

	struct address
	{
		engine_id engine_id_ = 0;
		uint64_t actor_id_ = 0;

		struct address_less
		{
			bool operator()(
				const address &lhs,
				const address &rhs) const
			{
				return (lhs.engine_id_ < rhs.engine_id_ ||
					lhs.actor_id_ < rhs.actor_id_);
			}
		};
		XENDEC(engine_id_, actor_id_);
	};

	/*
	| magic_code| msg length| from | to | msg type | msg |
	*/

#define RegistActorMsg(MsgName,...) \
	static constexpr char *_$_actor_msg_type_ = \
	"User::"#MsgName; \
	XENDEC(__VA_ARGS__);

#define RegistSysMsg(MsgName,...) \
	static constexpr char * _$_actor_msg_type_ = \
	"Sys::"#MsgName; \
	XENDEC(__VA_ARGS__);


	struct msg
	{
		address from_;
		address to_;
		std::string name_;
		std::string data_;
		XENDEC(from_, to_, name_, data_);
	};
	static constexpr int magic_code = 1213141516;

	struct msg_header
	{
		int magic_code = 1213141516;
		int length_ = 0;

		XENDEC(magic_code, length_);
	};

	template<typename T>
	inline constexpr char *get_msg_name()
	{
		return std::decay<T>::type::_$_actor_msg_type_;
	}

	template<typename T>
	inline constexpr char *get_msg_name(T*)
	{
		return std::decay<T>::type::_$_actor_msg_type_;
	}


	template<typename T>
	inline T to_msg(const std::string &data)
	{
		uint8_t *ptr = (uint8_t *)data.data();
		uint8_t *end = ptr + data.length();
		return xutil::endec::get<T>(ptr, end);
	}

	template<typename T>
	inline T to_msg(char *data, std::size_t len)
	{
		auto ptr = (uint8_t *)data;
		auto end = ptr + len;
		return xutil::endec::get<T>(ptr, end);
	}

	template<typename T>
	inline std::string to_data(const T &m)
	{
		std::string buffer;
		buffer.resize(m.xget_sizeof());
		uint8_t *ptr = (uint8_t*)buffer.data();
		uint8_t *end = ptr + buffer.size();
		xutil::endec::put(ptr, m);
		return buffer;
	}


	namespace notify 
	{
		struct address_error
		{
			address address_;
			RegistActorMsg(address_error, address_);
		};

		struct actor_close
		{
			address address_;
			RegistActorMsg(actor_close, address_);
		};

		struct actor_join
		{
			address address_;
			RegistActorMsg(actor_join, address_);
		};
	};

	namespace sys
	{
		struct init
		{
			bool _;
			RegistSysMsg(init, _);
		};

		struct watch 
		{
			bool _;
			RegistSysMsg(watch, _);
		};

		struct set_timer 
		{
			int32_t millis_;
			uint32_t timer_id_;
			RegistSysMsg(set_timer, millis_, timer_id_);
		};

		struct cancel_timer
		{
			uint32_t timer_id_;
			RegistSysMsg(cancel_timer, timer_id_);
		};

		struct timer_expire
		{
			uint32_t timer_id_;
			RegistSysMsg(timer_expire, timer_id_);
		};
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
		address get_address()
		{
			return addr_;
		}
	protected:
		virtual void init()
		{
			std::cout << "actor init" << std::endl;
		}

		template<typename T>
		void regist(const std::function<void(const address &from, T &&)>& func)
		{
			auto type = get_msg_name<T>();
			auto itr = msg_handles_.find(type);
			if (itr != msg_handles_.end())
				throw std::logic_error(type + std::string(" repeated regist"));
			msg_handles_.emplace(type, [func](const msg &m){
				func(m.from_, to_msg<T>(m.data_));
			});
		}

		uint32_t set_timer(int32_t millis, const std::function<void()> &func)
		{
			timer_handle_.emplace(timer_index_, func);

			sys::set_timer set_time_;
			set_time_.millis_ = millis;
			set_time_.timer_id_ = timer_index_;

			send({}, set_time_);
			return timer_index_++;
		}

		void cancel_timer(uint32_t timer_id_)
		{
			auto itr = timer_handle_.find(timer_id_);
			if (itr != timer_handle_.end())
			{
				timer_handle_.erase(itr);
				send({}, sys::cancel_timer{ timer_id_ });
			}
		}
		

		void watch(const address &addr)
		{
			send(addr, sys::watch());
		}

		void close()
		{
			for (auto &itr : observers_)
				send(itr, notify::actor_close{ itr });
			close_(addr_);
		}

		template<typename T>
		void send(const address &to, T &&_msg)
		{
			msg m;
			m.data_ = to_data(_msg);
			m.from_ = addr_;
			m.to_ = to;
			m.name_ = get_msg_name<T>();
			send_msg_(std::move(m));
		}
	private:
		friend class xengine;
		bool recv(msg &&m)
		{
			msg_queue_.write(std::move(m));
			return msg_queue_.flush();
		}

		void apply_all()
		{
			while (apply_once());
		}

		bool apply_once()
		{
			auto m = msg_queue_.read();
			if (!m.first)
				return false;
			apply(m.second);
			return msg_queue_.check_read();
		}

		void apply(const msg &m)
		{
			auto itr = msg_handles_.find(m.name_);
			if (itr != msg_handles_.end())
			{
				try
				{
					itr->second(m);
				}
				catch (const std::exception& e)
				{
					std::cout << e.what() << std::endl;
				}
			}
			else if (m.name_ == get_msg_name<sys::watch>())
			{
				observers_.insert(m.from_);
			}
			else if(m.name_ == get_msg_name<sys::init>())
			{
				init();
			}
		}
		using msg_handle_t = std::function<void(const msg &)>;
		std::map<std::string, msg_handle_t> msg_handles_;
		address addr_;
		xutil::ypipe<msg> msg_queue_;

		std::function<void(msg &&)> send_msg_;
		std::function<void(address )> close_;

		std::set<address, address::address_less> observers_;
		std::map<uint32_t, std::function<void()>> timer_handle_;

		uint32_t timer_index_ = 1;
	};

	class session 
		:public std::enable_shared_from_this<session>
	{
	public:
		using msgbox_t = xnet::proactor_pool::msgbox_t;

		struct session_id
		{
			struct session_id_less
			{
				bool operator()(const session_id&left, const session_id& right)const
				{
					return (left.peer_ < right.peer_ || left.self_ < right.self_);
				}
			};
			engine_id peer_;
			engine_id self_;
		};

		session(xnet::connection &&conn, msgbox_t &msgbox_)
			:msgbox_(msgbox_),
			conn_(std::move(conn))
		{
			init();
		}
		void send_msg(std::string &&msg)
		{
			std::lock_guard<std::mutex> lg(msg_queue_lock_);
			msg_queue_.write(std::move(msg));
			if (msg_queue_.flush())
				return;
			regist_send();
		}

		void send_msg(const msg& m)
		{
			using xutil::endec::put;

			std::string buffer;
			std::size_t len(0);

			len += sizeof(int);//magic code
			len += sizeof(int);//msg length
			len += m.xget_sizeof();
			buffer.resize(len);

			len -= 2 * sizeof(int);
			uint8_t *ptr = (uint8_t *)buffer.data();

			put(ptr, magic_code);
			put(ptr, len);
			put(ptr, m);
			send_msg(std::move(buffer));
		}

		void attach_conntion(xnet::connection &&conn)
		{
			if (conn_.valid())
				throw std::logic_error("connection is valid");

			conn_ = std::move(conn);
			init();
		}

		void set_msg_callback(const std::function<bool(msg &&)> &handle)
		{
			msg_callback_ = handle;
		}

	private:
		void init()
		{
			msg_queue_.check_read();
			conn_.regist_send_callback([this](std::size_t len) {

				if (!len)
					return close();
				send_msg();

			}).regist_recv_callback([this](char *data, std::size_t len) {
				
				uint8_t *ptr = (uint8_t*)data;
				uint8_t *end = ptr + len;

				if (!len)
					return close();

				if (status_ == e_head)
				{
					auto code = xutil::endec::get<int>(ptr, end);
					if (code != magic_code)
					{
						std::cout << "magic_code error " << code << std::endl;
						return close();
					}
					auto length = xutil::endec::get<int>(ptr, end);
					status_ = e_data;
					conn_.async_recv(length);
				}
				else if(status_ == e_data)
				{
					try
					{
						if (!msg_callback_(xutil::endec::get<msg>(ptr, end)))
						{
							notify::address_error error;

							auto m = to_msg<msg>(data, len);

							error.address_ = m.to_;
							m.data_ = to_data(error);
							m.to_ = m.from_;
							m.from_.engine_id_ = m.to_.engine_id_;
							m.from_.actor_id_ = 0;//
							m.name_ = get_msg_name(&error);

							send_msg(m);
						}
					}
					catch (const std::exception& e)
					{
						std::cout << e.what() << std::endl;
						return close();
					}
				}
			}).async_recv(sizeof(msg_header));

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

		std::function<void()> close_callback_;
		std::function<bool (msg &&)> msg_callback_;
		msgbox_t &msgbox_;
		std::mutex msg_queue_lock_;
		xutil::ypipe<std::string> msg_queue_;
		xnet::connection conn_;
	};


	class xengine
	{
	public:
		xengine(std::size_t io_threads = std::thread::hardware_concurrency())
			:proactor_pool_(io_threads),
			worker_size_(io_threads)
			
		{
		}
		void regist_actor()
		{
		}
		void bind(const std::string &ip, int port)
		{
			proactor_pool_.regist_accept_callback([this](xnet::connection &&conn){
				handle_conn(std::forward<xnet::connection>(conn));
			});
			proactor_pool_.bind(ip, port);
		}
		int workers()
		{
			return worker_size_;
		}
		void set_workers(int size)
		{
			worker_size_ = size;
		}
		void start()
		{
			proactor_pool_.start();
			worker_pool_.reset(new xutil::xworker_pool(worker_size_));
		}

		//create actor
		template<typename Actor, typename ...Args>
		typename std::enable_if<std::is_base_of<actor, Actor>::value, address>::type
			spawn(Args &&...args)
		{
			std::shared_ptr<actor> _actor(std::make_shared<Actor>(
				std::forward<Args>(args)...));

			_actor->addr_.actor_id_ = actor_index_++;
			_actor->addr_.engine_id_ = engine_id_;
			_actor->msg_queue_.check_read();
			_actor->send_msg_ = [this](msg &&m) {
				send_msg(std::move(m));
			};
			_actor->close_ = [this](address addr){
				del_actor(addr);
			};
			add_actor(_actor);
			_actor->send(_actor->addr_, sys::init());
			return _actor->addr_;
		}
	private:

		struct handshake
		{
			struct req
			{
				int magic_code_ = magic_code;
				engine_id from_;
				engine_id to_;
				int role;
				XENDEC(magic_code_, from_, to_, role);
			};

			struct resp 
			{
				enum result_t
				{
					e_ok,
					e_error_
				};
				int magic_code_ = magic_code;
				engine_id from_;
				engine_id to_;
				result_t result_;
				XENDEC(magic_code_, from_, to_, result_);
			};
		public:
			handshake(xnet::connection &&_conn)
				:conn_(std::move(_conn))
			{
				
			}
			void start()
			{
				conn_.regist_recv_callback(
					[this](char *data,std::size_t len) {

					if (!len)
						return close();
					uint8_t *ptr = reinterpret_cast<uint8_t*>(data);
					uint8_t *end = ptr + len;
					try
					{
						req re = xutil::endec::get<req>(ptr, end);
						if (re.magic_code_ != magic_code)
							throw std::logic_error("magic code error " +
								std::to_string(re.magic_code_));

						if (re.to_ != engine_id_ || re.from_ < engine_id_)
						{
							conn_.async_send(make_handshake_resp(engine_id_, 
								re.from_,
								resp::e_error_));
							return;
						}
						handle_handeshake_(re);
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

				conn_.async_recv(sizeof(req));
			}

			static std::string make_handshake_resp(engine_id from, 
				engine_id to, handshake::resp::result_t result)
			{
				handshake::resp resp;
				resp.from_ = from;
				resp.to_ = to;
				resp.result_ = result;
				std::string buffer;
				buffer.resize(resp.xget_sizeof());
				uint8_t *ptr = (uint8_t*)buffer.data();
				xutil::endec::put(ptr, resp);
				return buffer;
			}
			void close()
			{
				close_callback_();
				conn_.close();
			}

			std::function<void()> close_callback_;
			std::function<void(const handshake::req&)> handle_handeshake_;
			xnet::connection conn_;
			engine_id engine_id_;

			std::list<handshake>::iterator endpoints_itr_;
		};
		struct handshakes
		{
			std::mutex mutex_;
			std::list<handshake> endpoints_;

		};
		using handshake_itr = std::list<handshake>::iterator;


		struct actors
		{
			std::mutex mutex_;
			std::map<address, actor::ptr_t, address::address_less> actors_;;
		};

		struct sessions
		{
			std::mutex mutex_;
			std::map<session::session_id, 
				std::unique_ptr<session>, 
				session::session_id::session_id_less> sessions_;
		};

		void init()
		{
			worker_pool_.reset(new xutil::xworker_pool());
		}

		void send_msg(msg &&m)
		{
			if (m.from_.engine_id_ == engine_id_)
			{
				handle_msg(std::move(m));
			}
			else if(m.from_.engine_id_ == 0)
			{
				if (m.name_ == get_msg_name<sys::set_timer>())
				{

				}
				else if(m.name_ == get_msg_name<sys::cancel_timer>())
				{

				}
			}
			else
			{

			}
		}
		bool handle_msg(msg &&m)
		{
			std::lock_guard<std::mutex> lg(actors_.mutex_);
			auto itr = actors_.actors_.find(m.to_);
			if (itr == actors_.actors_.end())
				return false;
			if (!itr->second->recv(std::move(m)))
				post_apply_msg(itr->second);
			return true;
		}

		void post_apply_msg(const actor::ptr_t &actor_)
		{
			actor::wptr_t actor_wptr_ = actor_;
			auto job = [actor_wptr_, this] {
				apply_msg(actor_wptr_);
			};
			worker_pool_->add_job(std::move(job));
		}

		void apply_msg(const actor::wptr_t &actor_ptr)
		{
			auto actor_ = actor_ptr.lock();
			if (actor_) 
			{
				if (actor_->apply_once())
					post_apply_msg(actor_);
			}
		}

		void remove_handshake(const handshake_itr& itr)
		{
			std::lock_guard<std::mutex> lg(handshakes_.mutex_);
			handshakes_.endpoints_.erase(itr);
		}
	
		void handle_handshake(xnet::connection &&conn,
			const handshake::req &req)
		{
			session::session_id id;
			id.peer_ = req.from_;
			id.self_ = engine_id_;

			auto resp = handshake::make_handshake_resp(engine_id_, 
				req.from_, handshake::resp::e_ok);

			std::lock_guard<std::mutex> lg(sessions_.mutex_);
			auto itr = sessions_.sessions_.find(id);
			if (itr != sessions_.sessions_.end())
			{
				itr->second->attach_conntion(std::move(conn));
				itr->second->send_msg(std::move(resp));
			}
			else
			{
				std::unique_ptr<session> ptr;
				auto &mailbox = proactor_pool_.get_current_msgbox();
				ptr.reset(new session(std::move(conn), mailbox));
				ptr->send_msg(std::move(resp));
				ptr->set_msg_callback([this](msg &&m){
					return handle_msg(std::move(m));
				});
				sessions_.sessions_.emplace(id, std::move(ptr));
			}
		}

		void handle_conn(xnet::connection &&conn)
		{
			handshake _handshake(std::move(conn));
			handshake_itr itr;
			do 
			{
				std::lock_guard<std::mutex> lg(handshakes_.mutex_);
				itr = handshakes_.endpoints_.insert(
					handshakes_.endpoints_.end(), std::move(_handshake));
			} while (0);

			itr->engine_id_ = engine_id_;
			itr->endpoints_itr_ = itr;
			itr->close_callback_ = [itr, this]{
				remove_handshake(itr);
			};
			itr->handle_handeshake_ = [itr, this](const handshake::req &req) {

				handle_handshake(std::move(itr->conn_), req);
				remove_handshake(itr);
			};
			itr->start();
		}

		void add_actor(std::shared_ptr<actor> _actor)
		{
			std::lock_guard<std::mutex> lg(actors_.mutex_);
			actors_.actors_.emplace(_actor->get_address(), std::move(_actor));
		}

		void del_actor(const address &addr)
		{
			std::lock_guard<std::mutex> lg(actors_.mutex_);
			actors_.actors_.erase(addr);
		}

		uint64_t actor_index_ = 1;

		actors actors_;
		handshakes handshakes_;
		sessions sessions_;

		int worker_size_ = 0;
		xnet::proactor_pool proactor_pool_;
		std::unique_ptr<xutil::xworker_pool> worker_pool_;

		engine_id engine_id_ = 0;
	};
}