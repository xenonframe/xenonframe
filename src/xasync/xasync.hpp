#pragma once
#include <atomic>
#include <vector>
#include <functional>
#include "../xnet/xnet.hpp"
#include "../xcoroutine/xcoroutine.hpp"
#include "../xutil/function_traits.hpp"
#include "../xutil/xworker_pool.hpp"

namespace xasync
{
	class async
	{
	public:
		using functor = std::function<void()>;
		using async_done_callback = std::function<void(functor &&)>;
		async(const async_done_callback &callback, 
			 uint32_t worker_count = std::thread::hardware_concurrency())
			:async_done_callback_(callback),
			xworker_pool_(new xutil::xworker_pool(worker_count))
		{

		}

		template<typename Ret, typename ...Args, typename ...Params>
		Ret post(const std::function<Ret(Args...)> &action, Params &&...params)
		{
			std::function<void()> resume = xcoroutine::get_resume();
			Ret ret;
			auto action_wrap = [&]() {
				ret = std::move(action(std::forward<Params>(params)...));
				async_done_callback_(std::move(resume));
			};
			xworker_pool_->add_job(std::move(action_wrap));
			xcoroutine::yield();
			return ret;
		}

		template<typename Ret, typename ...Args, typename ...Params>
		Ret post(Ret(action)(Args...), Params && ...params)
		{
			std::function<void()> resume = xcoroutine::get_resume();
			Ret ret;
			auto action_wrap = [&] {
				ret = std::move(action(std::forward<Params>(params)...));
				async_done_callback_(std::move(resume));
			};
			xworker_pool_->add_job(std::move(action_wrap));
			xcoroutine::yield();
			return ret;
		}


		template<typename Ret, typename Class, typename ...Args, typename ...Params >
		Ret post(Ret(Class::*action)(Args...), Class &inst, Params && ...params)
		{
			std::function<void()> resume = xcoroutine::get_resume();
			Ret ret;
			auto action_wrap = [&]{
				ret = std::move((inst.*action)(std::forward<Params>(params)...));
				async_done_callback_(std::move(resume));
			};
			xworker_pool_->add_job(std::move(action_wrap));
			xcoroutine::yield();
			return ret;
		}
	private:
		std::function<void(functor &&)> async_done_callback_;
		std::unique_ptr<xutil::xworker_pool> xworker_pool_;
	};
	

}

