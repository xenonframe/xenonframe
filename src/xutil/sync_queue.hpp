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
			std::unique_lock<std::mutex> locker(mtex_);
			queue_.push(std::forward<T>(item));
			cv_.notify_one();
		}

		bool pop(T &t, int timeout_mills = 0)
		{
			std::unique_lock<std::mutex> locker(mtex_);
			if (queue_.empty())
			{
				if (timeout_mills == 0)
					cv_.wait(locker, [this] { return queue_.size(); });
				else
				{
					auto ret = cv_.wait_for(locker,
						std::chrono::milliseconds(timeout_mills),[this] {
						return !!queue_.size(); 
					});
					if (!ret)
						return false;
				}
			}
			t = std::move(queue_.front());
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
