#pragma once
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <errno.h>
#include <string.h>
#include <ucontext.h>
#endif
#include <cassert>
#include <list>
#include <functional>
#include <exception>
#include "../xutil/function_traits.hpp"
namespace xcoroutine
{
    struct xcoroutine_error:std::exception
    {
        xcoroutine_error()
        {
#ifdef _MSC_VER
            char errmsg[512];
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
				0, GetLastError(), 0, errmsg, 511, NULL);
            error_str_ = errmsg;
#else
            error_str_ = strerror(errno);
#endif
        }
        const char *what()
        {
            return error_str_.c_str();
        }
        std::string error_str_;
    };
	namespace detail
	{
#ifdef _MSC_VER
		thread_local LPVOID __main_fiber;

		inline void __stdcall entry(LPVOID lpParameter)
		{
			do
			{
				std::function<void()> *func = reinterpret_cast<
					std::function<void()>*>(lpParameter);
				assert(func);
				(*func)();
				SwitchToFiber(__main_fiber);
			} while (true);
		}

		struct routine_impl
		{
			routine_impl()
			{

			}
			~routine_impl()
			{
				if (fiber_)
					DeleteFiber(fiber_);
			}
			std::size_t stack_size_ = 64 * 1024;
			LPVOID fiber_ = nullptr;
			void init(std::function<void()> *func)
			{
				fiber_ = CreateFiberEx(stack_size_, 0, 
					FIBER_FLAG_FLOAT_SWITCH, entry, func);
                if(!fiber_)
                    throw xcoroutine_error();
			}
		};

		struct routine_swicher
		{
			void operator()(routine_impl &routine)
			{
				SwitchToFiber(routine.fiber_);
			}
		};

		struct routine_initer
		{
			void operator()()
			{
				__main_fiber = ConvertThreadToFiberEx(nullptr, 
					FIBER_FLAG_FLOAT_SWITCH);
				if (!__main_fiber)
                    throw xcoroutine_error();
			}
		};
		struct routine_uniniter
		{
			void operator()()
			{
				ConvertFiberToThread();
			}
		};

		struct yielder
		{
			void operator()(...)
			{
				SwitchToFiber(__main_fiber);
			}
		};
#else
		thread_local ucontext_t __main_ctx;
		thread_local ucontext_t *__current_ctx;
		inline void entry(void *param)
		{
			do
			{
				std::function<void()> *func = 
					reinterpret_cast<std::function<void()>*>(param);
				assert(func);
				(*func)();
				if (swapcontext(__current_ctx, &__main_ctx) == -1)
					throw xcoroutine_error();

			} while (true);
		}

		struct routine_impl
		{
			routine_impl()
			{

			}
			~routine_impl()
			{
				if (stack_)
					delete[]stack_;
			}
			std::size_t stack_size_ = 64 * 1024;
			char *stack_ = nullptr;
			ucontext_t ctx_;
			void init(std::function<void()> *func)
			{
				if (getcontext(&ctx_) == -1)
                    throw xcoroutine_error();
				stack_ = new char[stack_size_];
				ctx_.uc_stack.ss_sp = stack_;
				ctx_.uc_stack.ss_size = stack_size_;
				ctx_.uc_link = &__main_ctx;

				makecontext(&ctx_, (void(*)(void))entry, 1, func);
			}
		};

		struct routine_swicher
		{
			void operator()(routine_impl &routine)
			{
				__current_ctx = &routine.ctx_;
				if (swapcontext(&__main_ctx, &routine.ctx_) == -1)
					throw xcoroutine_error();
			}
		};

		struct routine_initer
		{
			void operator()()
			{
			}
		};
		struct routine_uniniter
		{
			void operator()()
			{
			}
		};

		struct yielder
		{
			void operator()(routine_impl &routine)
			{
				__current_ctx = nullptr;
				if(swapcontext(&routine.ctx_, &__main_ctx) == -1)
                    throw xcoroutine_error();
			}
		};
#endif
	}

	struct xroutine
	{
		std::function<void()> entry_;
		std::function<void()> func_;
		std::function<void()> done_;
		detail::routine_impl impl_;
		xroutine()
		{
			init();
		}
		void init()
		{
			entry_ = [this]
			{
				func_();
				done_();
			};
			impl_.init(&entry_);
		}
	};
	struct xroutine_creater :std::list<xroutine*>
	{
		xroutine_creater()
		{
			detail::routine_initer()();
		}
		~xroutine_creater()
		{
			detail::routine_uniniter()();
			for (auto &itr : *this)
			{
				delete itr;
			}
		}
		template<typename Func>
		xroutine* create(Func &&func)
		{
			xroutine* coro = nullptr;
			if (size())
			{
				coro = front();
				pop_front();
			}
			else
			{
				coro = new xroutine;
				coro->done_ = [this, coro] { push_back(coro); };
			}
			coro->func_ = std::forward<Func>(func);
			return coro;
		}
	};

	struct {
		xroutine *current_routine_ = nullptr;
		xroutine_creater routine_creater;
	}static thread_local thread_local_;

	struct routine_swicher
	{
		void operator()(xroutine *coro)
		{
			thread_local_.current_routine_ = coro;
			detail::routine_swicher()(coro->impl_);
		}
	};

	static inline std::function<void()> get_resume()
	{
		xroutine *coro = thread_local_.current_routine_;
		assert(coro);
		return [coro] { routine_swicher()(coro); };
	}

	static inline void yield()
	{
		xroutine *coro = thread_local_.current_routine_;
		assert(coro);
		detail::yielder()(coro->impl_);
	}
	template<typename T>
	static inline void yield(T &resume)
	{
		xroutine *coro = thread_local_.current_routine_;
		assert(coro);
		resume = [coro] { routine_swicher()(coro); };
		detail::yielder()(coro->impl_);
	}

	template<typename Func>
	static inline void create(Func &&func)
	{
		xroutine *coro = thread_local_.routine_creater.create(std::forward<Func>(func));
		assert(coro);
		routine_swicher()(coro);
	}

	template<typename ...Args, typename ...Params>
	auto apply(const std::function<void(Args...)> &async_do, Params &&...params)
	{
		using func_traits = xutil::function_traits<std::function<void(Args...)>>;
		using callback_func_traits = xutil::function_traits <typename func_traits:: template args<func_traits::arity - 1>::type> ;
		typename callback_func_traits::stl_function_type func;
		typename callback_func_traits::tuple_type result;
		std::function<void()> resume_func;
		bool is_done = false;
		func = [&](auto &&... args) {
			result = std::forward_as_tuple(std::forward<decltype(args)>(args)...);
			is_done = true;
			if (resume_func)
				resume_func();
		};
		async_do(std::forward<Params>(params)...,std::move(func));
		if (!is_done)
			xcoroutine::yield(resume_func); 
		return result;
	}
}
