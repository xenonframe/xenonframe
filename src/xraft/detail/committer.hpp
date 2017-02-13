#pragma once
namespace xraft
{
	namespace detail
	{
		template<typename item = std::function<void()>>
		class committer
		{
		public:
			committer()
			{
				worker_ = std::thread([this] { 
					run(); });
			}
			~committer()
			{
				stop();
			}
			void push(item &&_item)
			{
				std::lock_guard<std::mutex> lock(mtx_);
				queue_.emplace(std::move(_item));
				cv_.notify_all();
			}
			void stop()
			{
				is_stop_ = true;
				if (worker_.joinable())
					worker_.join();
			}
		private:
			bool pop(item &_item)
			{
				std::unique_lock<std::mutex> lock(mtx_);
				if (queue_.empty())
				{
					 auto res = cv_.wait_for(lock,std::chrono::milliseconds(1000));
					 if (res == std::cv_status::timeout || queue_.empty())
						 return false;
				}
				_item = std::move(queue_.front());
				queue_.pop();
				return true;
			}
			void run()
			{
				do
				{
					item _item;
					if (pop(_item))
						_item();
				} while (is_stop_ == false);
			}
			bool is_stop_ = false;
			std::queue<item> queue_;
			std::condition_variable cv_;
			std::mutex mtx_;
			std::thread worker_;
		};
	}
}