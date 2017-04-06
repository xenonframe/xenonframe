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
		}

		bool pop(T &t)
		{
			std::unique_lock<std::mutex> locker(mtex_);
			if (queue_.empty())
				return false;
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
		std::queue<T> queue_;
	};
}
