#pragma once
#include <utility>
#include "yqueue.hpp"
namespace xutil
{
	template <typename T, int N> class ypipe
	{
	public:
		inline ypipe()
		{
			queue_.push();
			r_ = w_ = f_ = &queue_.back();
			c_ = (&queue_.back());
		}
		inline virtual ~ypipe()
		{}
		inline void write(T &&value_, bool complete_ = true)
		{
			queue_.back() = std::forward<T>(value_);
			queue_.push();
			if (complete_)
				f_ = &queue_.back();
		}
		inline bool flush()
		{
			if (w_ == f_)
				return true;
			T *expected = w_;
			c_.compare_exchange_strong(expected, f_);
			if (expected != w_) {
				c_ = f_;
				w_ = f_;
				return false;
			}
			w_ = f_;
			return true;
		}
		inline bool check_read()
		{
			if (&queue_.front() != r_ && r_)
				return true;
			T *l_value = &queue_.front();
			c_.compare_exchange_strong(l_value, nullptr);
			r_ = l_value;
			if (&queue_.front() == r_ || !r_)
				return false;
			return true;
		}
		inline std::pair<bool, T> read()
		{
			if (!check_read())
				return {false, T()};
			std::pair<bool, T> res = { true, std::move(queue_.front()) };
			queue_.pop();
			return res;
		}
	private:
		yqueue <T, N> queue_;
		T *w_ = nullptr;
		T *r_ = nullptr;
		T *f_ = nullptr;
		std::atomic<T*> c_;
		ypipe(const ypipe&) = delete;
		const ypipe &operator = (const ypipe&) = delete;
	};
}
