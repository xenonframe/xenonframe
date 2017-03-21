// Copyright Leon Timmermans 2012-2017

#ifndef __ACTOR_H__
#define __ACTOR_H__

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <list>
#include <tuple>

namespace actor
{
	class queue 
	{
		class message_base 
		{
		public:
			virtual void* get(const std::type_info& info) = 0;
			virtual ~message_base() {}
			template<typename T> T* to() 
			{
				return static_cast<T*>(get(typeid(std::decay_t<T>)));
			}
		};
		template<typename T> class message : public message_base {
			T _value;
		public:
			message(T value)
				: _value(std::move(value))
			{}
			virtual void* get(const std::type_info& info) {
				if (typeid(T) == info)
					return &_value;
				else
					return nullptr;
			}
		};
		template<typename T> static std::unique_ptr<message_base> make_message(T value) 
		{
			return std::make_unique<message<std::decay_t<T>>>(std::move(value));
		}

		template<class Function, class Tuple, std::size_t... I>
		static void apply_impl(Function&& f, Tuple&& t, std::index_sequence<I...>)
		{
			f(std::get<I>(std::forward<Tuple>(t))...);
		}

		template<class Function, class Tuple> 
		static void apply(Function&& f, Tuple&& t)
		{
			apply_impl(std::forward<Function>(f), std::forward<Tuple>(t), std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
		}

		template<typename T>
		struct function_traits;

		template<typename ClassType, typename ReturnType, typename... Args>
		struct function_traits<ReturnType(ClassType::*)(Args...) const>
		{
			using args = std::tuple<std::decay_t<Args>...>;
		};


		template<typename T> 
		struct function_traits :
			public function_traits<decltype(&T::operator())> 
		{
		};
		

		template<typename Callback> 
		static bool match_if(std::unique_ptr<message_base>&, const Callback&) 
		{
			return false;
		}

		template<typename Callback, typename Head, typename... Tail>
		static bool match_if(std::unique_ptr<message_base>& any, const Callback& callback, const Head& head, const Tail&... tail)
		{
			using arg_type = typename function_traits<Head>::args;
			if (arg_type* pointer = any->to<arg_type>()) 
			{
				arg_type value = std::move(*pointer);
				callback();
				apply(head, std::move(value));
				return true;
			}
			else
				return match_if(any, callback, tail...);
		}

		std::mutex mutex;
		std::condition_variable cond;
		std::queue<std::unique_ptr<message_base>> incoming;
		std::list<std::unique_ptr<message_base>> pending;
		std::vector<std::weak_ptr<queue>> monitors;
		std::atomic<bool> living;
		queue(const queue&) = delete;
		queue& operator=(const queue&) = delete;

	public:
		queue()
			: mutex()
			, cond()
			, incoming()
			, pending()
			, monitors()
			, living(true)
		{ }
		template<typename T> void push(T&& value) {
			std::lock_guard<std::mutex> lock(mutex);
			if (!living)
				return;
			incoming.push(make_message<T>(std::move(value)));
			cond.notify_one();
		}
		template<typename... Args> void match(const Args&... matchers)
		{
			match_with([this](auto& lock, const auto& check) -> bool {
				cond.wait(lock, check); 
				return true;
			}, matchers...);
		}

		template<typename Clock, typename Rep, typename Period, typename... Args>
		bool match_until(const std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>>& until, const Args&... args)
		{
			return match_with([this, &until](auto& lock, const auto& check) { return cond.wait_until(lock, until, check); }, args...);
		}

		bool add_monitor(const std::shared_ptr<queue>& monitor) 
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (living)
				monitors.push_back(monitor);
			return living;
		}
		bool alive() const 
		{
			return living;
		}

		template<typename... Args> 
		void mark_dead(Args&&... args)
		{
			std::lock_guard<std::mutex> lock(mutex);
			living = false;
			pending.clear();
			while (!incoming.empty())
				incoming.pop();
			const auto testament = std::make_tuple(std::forward<Args>(args)...);
			for (const auto& monitor : monitors)
				if (const auto strong = monitor.lock())
					strong->push(testament);
			monitors.clear();
		}
	private:
		template<typename Waiter, typename... Args>
		bool match_with(const Waiter& waiter, const Args&... matchers)
		{
			static_assert(sizeof...(Args) != 0, "Can't call receive without arguments");

			for (auto current = pending.begin(); current != pending.end(); ++current)
				if (match_if(*current, [&] { pending.erase(current); }, matchers...))
					return true;
			std::unique_lock<std::mutex> lock(mutex);
			while (1) 
			{
				if (!waiter(lock, [&] { return !incoming.empty(); }))
					return false;
				else if (match_if(incoming.front(), [&] { incoming.pop(); lock.unlock(); }, matchers...))
					return true;
				else 
				{
					pending.push_back(std::move(incoming.front()));
					incoming.pop();
				}
			}
		}
	};

	namespace hidden 
	{
		extern const thread_local std::shared_ptr<queue> mailbox = std::make_shared<queue>();
	}

	class handle 
	{
		std::shared_ptr<queue> mailbox;
	public:
		handle()
		{
		}
		explicit handle(const std::shared_ptr<queue>& other)noexcept 
			: mailbox(other) 
		{
		}
		template<typename... Args> void send(Args&&... args) const 
		{
			mailbox->push(std::make_tuple(std::forward<Args>(args)...));
		}
		bool monitor() const 
		{
			return mailbox->add_monitor(hidden::mailbox);
		}
		bool alive() const noexcept
		{
			return mailbox->alive();
		}
		friend void swap(handle& left, handle& right) noexcept 
		{
			swap(left.mailbox, right.mailbox);
		}
		friend bool operator==(const handle& left, const handle& right) 
		{
			return left.mailbox == right.mailbox;
		}
		friend bool operator!=(const handle& left, const handle& right)
		{
			return left.mailbox != right.mailbox;
		}
		friend bool operator<(const handle& left, const handle& right)
		{
			return left.mailbox.get() < right.mailbox.get();
		}
	};

	namespace hidden 
	{
		extern const thread_local handle self_var(hidden::mailbox);
	}
	static inline const handle& self() 
	{
		return hidden::self_var;
	}

	template<typename... Matchers>
	void receive(const Matchers&... matchers)
	{
		hidden::mailbox->match(matchers...);
	}

	template<typename Condition, typename... Matchers> 
	void receive_while(const Condition& condition, const Matchers&... matchers)
	{
		while (condition)
			receive(matchers...);
	}

	template<typename Clock, typename Rep, typename Period, typename... Matchers> 
	bool receive_until(const std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>>& until, Matchers&&... matchers)
	{
		return hidden::mailbox->match_until(until, matchers...);
	}

	template<typename Rep, typename Period, typename... Matchers>
	bool receive_for(const std::chrono::duration<Rep, Period>& until, Matchers&&... matchers)
	{
		return receive_until(std::chrono::steady_clock::now() + until, matchers...);
	}

	struct exit {};
	struct error {};

	template<typename Func, typename... Args> handle spawn(Func&& func, Args&&... params)
	{
		std::promise<handle> promise;
		auto callback = [&promise](auto function, auto... args)
		{
			promise.set_value(self());
			try
			{
				function(std::forward<decltype(args)>(args)...);
			}
			catch (...) 
			{
				hidden::mailbox->mark_dead(error(), self(), std::current_exception());
				return;
			}
			hidden::mailbox->mark_dead(exit(), self());
		};
		std::thread(callback, std::forward<Func>(func), std::forward<Args>(params)...).detach();
		return promise.get_future().get();
	}
}

#endif
