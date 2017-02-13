#pragma once
#include "xnet.hpp"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <list>
namespace xnet
{
	class proactor_pool :xutil::no_copy_able
	{
		
	public:
		typedef std::function<void(connection &&) > accept_callback_t;
		typedef std::function<void(void)> callbck_t;
		using msgbox_t = msgbox<std::function<void()>>;

		proactor_pool(std::size_t io_threads = std::thread::hardware_concurrency())
			:size_(io_threads)
		{
			if (!size_)
				size_ = std::thread::hardware_concurrency() * 2;
			proactors_.reserve(size_);
		}
		std::size_t get_size()
		{
			return size_;
		}
		proactor &get_proactor(std::size_t index)
		{
			if (index >= proactors_.size())
				throw std::out_of_range("index >= proactors_.size()");
			return proactors_[index];
		}
		template<typename T>
		std::size_t set_timer(uint32_t timeout, T &&func)
		{
			return get_current_proactor().set_timer(timeout, std::forward<T>(func));
		}
		void cancel_timer(std::size_t timer_id_)
		{
			return get_current_proactor().cancel_timer(timer_id_);
		}
		proactor &get_current_proactor()
		{
			return *current_proactor_store();
		}
		msgbox_t &get_current_msgbox()
		{
			return *current_msgbox_store();
		}
		proactor_pool &post(std::function<void()> &&handle, std::size_t index = 0)
		{
			if (index >= msgboxs_.size())
				throw std::out_of_range("index >= msgboxs_.size()");
			msgboxs_[index]->send(std::move(handle));
			return *this;
		}
		proactor_pool &bind(const std::string &ip, int port)
		{
			is_bind_ = true;
			ip_ = ip;
			port_ = port;
			if (is_start_)
			{
				xnet_assert(msgboxs_.size());
				auto func = [this] {
					acceptor_.reset(new xnet::acceptor(get_current_proactor().get_acceptor()));
					acceptor_->regist_accept_callback(std::bind(&
						proactor_pool::accept_callback,
						this, std::placeholders::_1));
					xnet_assert(acceptor_->bind(ip_, port_));
				};
				msgboxs_[0]->send(func);
			}
			return *this;
		}
		void start()
		{
			for (std::size_t i = 0; i < size_; i++)
				proactors_.emplace_back();
			start_proactors();
			is_start_ = true;
			if(is_bind_)
				bind(ip_, port_);
		}
		void stop()
		{
			for (auto &itr : proactors_)
				itr.stop();
			for (auto &itr : workers_)
			{
				if (itr.joinable())
					itr.join();
			}
				
		}
		proactor_pool &regist_accept_callback(accept_callback_t callback )
		{
			accept_callback_ = callback;
			return *this;
		}
		proactor_pool & regist_run_before(callbck_t callback)
		{
			run_before_callbck_ = callback;
			return *this;
		}
		proactor_pool &regist_run_end(callbck_t callbck)
		{
			run_end_callbck_ = callbck;
			return *this;
		}
	private:
		proactor* current_proactor_store(proactor *_proactor = nullptr)
		{
			static thread_local proactor *current_proactor_ = nullptr;
			if (_proactor)
				current_proactor_ = _proactor;
			return current_proactor_;
		}
		
		msgbox_t *current_msgbox_store(msgbox_t *_msgbox = nullptr)
		{
			static thread_local msgbox_t *msgbox = nullptr;
			if (_msgbox)
				msgbox = _msgbox;
			return msgbox;
		}

		void start_proactors()
		{
			std::mutex mtx;
			std::condition_variable sync;
			msgboxs_.reserve(size_);
			workers_.reserve(size_);
			for (std::size_t i = 0; i < size_; ++i)
			{
				proactor &pro = proactors_[i];
				workers_.emplace_back([&] 
				{
					current_proactor_store(&pro);
					msgboxs_.emplace_back(new msgbox_t(pro));
					msgboxs_.back()->regist_notify([&pro,this,i]
					{
						do
						{
							auto &msgbox = msgboxs_[i];
							auto item = msgbox->recv();
							if (!item.first)
								break;
							item.second();
						} while (true);
					});
					msgboxs_.back()->regist_inited_callback([&] 
					{
						std::unique_lock<std::mutex> locker(mtx);
						sync.notify_one();
					});
					current_msgbox_store(msgboxs_.back().get());
					if(run_before_callbck_)
						run_before_callbck_();
					pro.run();
					if (run_end_callbck_)
						run_end_callbck_();
				});
				std::unique_lock<std::mutex> locker(mtx);
				sync.wait(locker);
			}
		}
		connection get_connection()
		{
			std::lock_guard<std::mutex> locker(connections_mutex_);
			xnet::connection _conn = std::move(connections_.back());
			connections_.pop_back();
			return std::move(_conn);
		}
		void add_connection(connection && conn)
		{
			std::lock_guard<std::mutex> locker(connections_mutex_);
			connections_.emplace_back(std::move(conn));
		}
		void accept_callback(xnet::connection && conn)
		{
			add_connection(std::move(conn));
			auto func = [this]()mutable {
				auto _conn = get_connection();
				get_current_proactor().regist_connection(_conn);
				accept_callback_(std::move(_conn));
			};
			msgboxs_[++mbox_index_ % msgboxs_.size()]->send(std::move(func));
		}
		std::atomic_bool is_bind_ { false };
		std::atomic_bool is_start_ { false };
		std::string ip_;
		int port_ = 0;
		std::size_t size_ = 0;
		
		std::vector<std::thread> workers_;
		std::vector<proactor> proactors_;
		
		accept_callback_t accept_callback_;
		std::unique_ptr<acceptor> acceptor_;
		
		int64_t mbox_index_ = 0;
		std::vector<std::unique_ptr<msgbox_t>> msgboxs_;

		callbck_t run_before_callbck_;
		callbck_t run_end_callbck_;

		std::mutex connections_mutex_;
		std::list<xnet::connection> connections_;
	};
}
