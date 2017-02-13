#pragma once
#include <chrono>
namespace xnet
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
	class timer_manager :
		public std::multimap<
		std::chrono::high_resolution_clock::time_point, xtimer>
	{
	public:
		typedef std::size_t timer_id;
		timer_manager()
		{

		}
		int64_t do_timer()
		{
			if (empty())
				return 0;
			auto itr = begin();
			duration_caster<milliseconds, high_resolution_clock::duration> caster;
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
}
