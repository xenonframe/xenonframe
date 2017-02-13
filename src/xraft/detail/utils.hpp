#pragma once
namespace xraft
{
namespace detail
{
	namespace utils
	{
		using lock_guard = std::lock_guard<std::mutex>;

		template<typename T>
		struct lock_queue
		{
			bool empty()
			{
				lock_guard lock(mtx_);
				return queue_.empty();
			}
			bool pop(T &val)
			{
				lock_guard lock(mtx_);
				if (queue_.empty())
					return false;
				val = std::move(queue_.front());
				queue_.pop();
				return true;
			}
			void push(T &&val)
			{
				lock_guard lock(mtx_);
				queue_.emplace(std::forward<T>(val));
			}
		private:
			std::queue<T> queue_;
			std::mutex mtx_;
		};
	}
}
}