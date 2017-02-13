#pragma once
namespace xraft
{
	namespace detail
	{
		using namespace std::chrono;
		class timer
		{
		public:
			timer()				
			{
			}

			~timer()
			{
				stop();
			}
			int64_t set_timer(int64_t timeout, std::function<void()> &&callback)
			{
				utils::lock_guard lock(mtx_);
				auto timer_point = high_resolution_clock::now()
					+ high_resolution_clock::duration(milliseconds(timeout));
				auto timer_id = gen_timer_id();
				actions_.emplace(std::piecewise_construct, 
					std::forward_as_tuple(timer_point), 
					std::forward_as_tuple(timer_id,std::move(callback)));
				cv_.notify_one();
				return timer_id;
			}
			void cancel(int64_t timer_id)
			{
				utils::lock_guard lock(mtx_);
				for (auto &itr = actions_.begin(); itr != actions_.end(); itr++)
				{
					if (itr->second.first == timer_id)
					{
						actions_.erase(itr);
						cv_.notify_one();
						return;
					}
				}
			}
			void start()
			{
				worker_ = std::thread([this] { run(); });
			}
			void stop()
			{
				if (worker_.joinable())
				{
					stop_ = true;
					cv_.notify_one();
					worker_.join();
				}
			}
		private:
			void run()
			{
				while (!stop_)
				{
					std::unique_lock<std::mutex> lock(mtx_);
					if (actions_.empty())
					{
						cv_.wait_for(lock, std::chrono::milliseconds(500));
						continue;
					}
					auto itr = actions_.begin();
					auto timeout_point = itr->first;
					if (timeout_point > high_resolution_clock::now())
					{
						cv_.wait_until(lock, timeout_point);
						continue;
					}
					auto action = std::move(itr->second.second);
					actions_.erase(itr);
					lock.unlock();
					action();
				}
			}
			int64_t gen_timer_id()
			{
				return ++timer_id_;
			}
			bool stop_ = false;
			std::condition_variable cv_;
			std::mutex mtx_;
			int64_t timer_id_ = 1;
			std::multimap<high_resolution_clock::time_point, 
				std::pair<int64_t, std::function<void()>>> actions_;
			std::thread worker_;
		};
	}
}