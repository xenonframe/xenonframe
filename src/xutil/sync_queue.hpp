#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
namespace xutil
{
	template<typename T>
	class sync_queue
	{
	public:
		sync_queue()
		{
		}

		void push(T &&item)
		{
			std::lock_guard<std::mutex> locker(mtex_);
			queue_.push(std::forward<T>(item));
			cv_.notify_one();
		}

		bool pop(T &job, int timeout_mills = 0)
		{
			std::unique_lock<std::mutex> locker(mtex_);
			if (queue_.empty())
			{
				cv_.wait_for(locker,std::chrono::milliseconds(timeout_mills));
				if (queue_.empty())
					return false;
			}
			job = std::move(queue_.front());
			queue_.pop();
			return true;
		}
		std::size_t jobs()
		{
			std::unique_lock<std::mutex> locker(mtex_);
			return queue_.size();
		}
		bool emtry()
		{
			std::unique_lock<std::mutex> locker(mtex_);
			return queue_.empty();
		}
	private:
		std::mutex mtex_;
		std::condition_variable cv_;
		std::queue<T> queue_;
	};
}
