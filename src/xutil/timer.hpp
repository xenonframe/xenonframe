#pragma once
#include <chrono>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace xutil
{
	using namespace std::chrono;
	struct xtimer
	{
		std::size_t timer_id_;
		std::size_t timeout_;
		std::function<bool()> timer_callback_;
	};

	template<typename T, typename D>
	struct duration_caster
	{
		T operator()(D d)
		{
			return std::chrono::duration_cast<T>(d);
		}
	};

	template<typename CLOCK>
	struct time_pointer
	{
		typename CLOCK::time_point operator()()
		{
			return CLOCK::now();
		}
	};
	using timer_id = std::size_t ;

	class timer_manager :
		public std::multimap<
		std::chrono::high_resolution_clock::time_point, xtimer>
	{
	public:
		timer_manager()
		{

		}
		int64_t do_timer()
		{
			if (empty())
				return 0;
			auto itr = begin();
			duration_caster<milliseconds,
				high_resolution_clock::duration> caster;

			time_pointer<high_resolution_clock> time_point;
			auto now = time_point();
			while (size() &&
				(caster(itr->first.time_since_epoch()) <= 
					caster(now.time_since_epoch())))
			{
				xtimer timer = itr->second;
				erase(itr);
				bool repeat = timer.timer_callback_();
				now = time_point();
				if (repeat)
				{
					auto point = now + high_resolution_clock::
						duration(milliseconds(timer.timeout_));
					insert(std::make_pair(point, timer));
				}
				if (size())
					itr = begin();
				else
					return 0;
			}
			return (caster(itr->first.time_since_epoch()) - 
				caster(now.time_since_epoch())).count();
		}
		template<typename T>
		timer_id set_timer(
			std::size_t timeout, 
			T &&timer_callback)
		{
			auto timer_point = std::chrono::high_resolution_clock::now()
				+ std::chrono::high_resolution_clock::
				duration(std::chrono::milliseconds(timeout));
			xtimer timer;
			timer.timer_callback_ = std::forward<T>(timer_callback);
			timer.timeout_ = timeout;
			timer.timer_id_ = ++next_id_;
			insert(std::make_pair(timer_point, timer));
			return next_id_;
		}
		void cancel_timer(std::size_t id)
		{
			for (auto itr = begin();itr !=end(); ++itr)
			{
				if (id == itr->second.timer_id_)
				{
					erase(itr);
					return;
				}
			}
		}
	private:
		std::size_t next_id_ = 0;
	};

	class timer
	{
	public:
		timer()
		{
		}
		~timer()
		{
		}
		template<typename T>
		timer_id set_timer(
			std::size_t timeout,
			T &&timer_callback)
		{
			std::lock_guard<std::mutex> lg(mutex_);
			auto id = timer_manager_.set_timer(timeout, 
				std::forward<T>(timer_callback));
			cv_.notify_one();
			return id;
		}
		void cancel_timer(std::size_t id)
		{
			return timer_manager_.cancel_timer(id);
		}
		void start()
		{
			worker_ = std::thread([this] {
				do
				{
					std::unique_lock<std::mutex> lg(mutex_);
					auto delay = timer_manager_.do_timer();
					if (!delay)
						delay = 1000;
					cv_.wait_for(lg, std::chrono::milliseconds(delay));

				} while (!is_stop);
			});
		}
		void stop()
		{
			is_stop = true;
			cv_.notify_one();
			worker_.join();
		}
	private:
		timer_manager timer_manager_;
		std::mutex mutex_;
		std::condition_variable cv_;
		std::thread worker_;
		bool is_stop = false;
	};
}
