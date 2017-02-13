#pragma once
namespace xhttp_server
{
	namespace detail
	{
		class async_wrap
		{
		public:
			using functor_t = std::function<void()>;
			using done_callback_t = std::function<void(functor_t&&)>;
			async_wrap(xnet::proactor_pool::msgbox_t *inst, uint32_t worker_count)
				:msgbox_(inst)
			{
				async_.reset(new xasync::async([this](functor_t &&done) {
					msgbox_->send(std::move(done));
				}, worker_count));
			}

			template<typename Ret, typename ...Args, typename ...Params>
			Ret post(const std::function<Ret(Args...)> &action, Params &&...params)
			{
				return async_->post(action, std::forward<Params>(params)...);
			}

			template<typename Ret, typename ...Args, typename ...Params>
			Ret post(Ret(action)(Args...), Params && ...params)
			{
				return async_->post(action, std::forward<Params>(params)...);
			}


			template<typename Ret, typename Class, typename ...Args, typename ...Params >
			Ret post(Ret(Class::*action)(Args...), Class &inst, Params && ...params)
			{
				return async_->post(action, inst, std::forward<Params>(params)...);
			}

		private:
			std::unique_ptr<xasync::async> async_;
			xnet::proactor_pool::msgbox_t *msgbox_ = nullptr;
		};

		inline async_wrap* async_instance_cache(
			xnet::proactor_pool::msgbox_t *msgbox = nullptr,
			uint32_t worker_count_ = 0, 
			bool reset = false)
		{
			static thread_local std::unique_ptr<async_wrap> _async_wrap_;
			if (reset)
			{
				_async_wrap_.reset(nullptr);
				return nullptr;
			}
			if (msgbox)
				_async_wrap_.reset(new async_wrap(msgbox, worker_count_));
			return _async_wrap_.get();
		}
	}
	inline void init_async(xnet::proactor_pool::msgbox_t &msgbox, uint32_t worker_count_)
	{
		detail::async_instance_cache(&msgbox, worker_count_, false);
	}

	inline void unint_async()
	{
		detail::async_instance_cache(0, 0, true);
	}

	template<typename ...Args>
	auto async(Args&& ...args)
	{
		return detail::async_instance_cache()->post(std::forward<Args>(args)...);
	}
}