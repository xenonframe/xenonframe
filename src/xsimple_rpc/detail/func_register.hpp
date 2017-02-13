#pragma once
namespace xsimple_rpc
{
	struct lock_free 
	{
		void lock()
		{

		}
		void unlock()
		{

		}
	};
	template<typename mutex = lock_free>
	class func_register
	{
	public:

		func_register()
		{

		}
		template<typename String, typename FuncType>
		void regist(String&&funcname, FuncType &&func)
		{
			regist_help(std::forward<String>(funcname), xutil::to_function(std::move(func)));
		}
		std::string invoke(const std::string &func_name, uint8_t *ptr, uint8_t *const end)
		{
			auto func = get_function(func_name);
			return func(ptr, end);
		}
	private:
		template<typename Ret, typename ...Args>
		typename std::enable_if<sizeof...(Args) >= 2, void>::type
			regist_help(const std::string &funcname, std::function<Ret(Args...)> &&func)
		{
			return regist_impl(std::move(funcname), std::move(func), std::make_index_sequence<sizeof ...(Args)>{});
		}
		template<typename ...Args>
		typename std::enable_if<sizeof...(Args) >= 2, void>::type
			regist_help(const std::string &funcname, std::function<void(Args...)> &&func)
		{
			std::function<std::string(Args...)> wrapper_func(
				[func](Args&&... args)->std::string {
				func(std::forward<Args>(args)...);
				return{};
			});
			return regist_impl(funcname, std::move(wrapper_func), std::make_index_sequence<sizeof ...(Args)>());
		}
		template<typename Ret, typename ...Args>
		typename std::enable_if<sizeof...(Args) == 1, void>::type
			regist_help(const std::string &funcname, std::function<Ret(Args...)> &&func)
		{
			return regist_impl(std::move(funcname), std::move(func));
		}
		template<typename ...Args>
		typename std::enable_if<sizeof...(Args) == 1, void>::type
			regist_help(const std::string &funcname, std::function<void(Args...)> &&func)
		{
			std::function<std::string(Args...)> wrapper_func(
				[func](Args&&... args)->std::string {
				func(std::forward<Args>(args)...);
				return{};
			});
			return regist_impl(funcname, std::move(wrapper_func));
		}
		template<typename Ret>
		void regist_help(const std::string &funcname, std::function<Ret(void)> &&func)
		{
			return regist_impl(std::move(funcname), std::move(func));
		}
		void regist_help(const std::string &funcname, std::function<void()> &&func)
		{
			return regist_impl(std::move(funcname), std::move([func]()->std::string {
				func();
				return{};
			}));
		}
		void regist_impl(const std::string &funcname, std::function<void()> &&func)
		{
			auto func_impl = [func](uint8_t*, uint8_t *const) ->std::string
			{
				func();
				static std::string ret;
				std::string buffer;
				buffer.resize(detail::endec::get_sizeof(ret));
				auto ptr = (uint8_t *)(buffer.data());
				detail::endec::put(ptr, ret);
				return {};
			};
			std::lock_guard<mutex> locker(mtex_);
			functions_.emplace(funcname, std::move(func_impl));
		}
		template<typename Ret>
		void regist_impl(const std::string &funcname, std::function<Ret()> &&func)
		{
			auto func_impl = [func](uint8_t*, uint8_t *const) ->std::string
			{
				auto result = func();
				std::string buffer;
				buffer.resize(detail::endec::get_sizeof(result));
				auto ptr = (uint8_t *)(buffer.data());
				detail::endec::put(ptr, result);
				return std::move(buffer);
			};
			std::lock_guard<mutex> locker(mtex_);
			functions_.emplace(funcname, std::move(func_impl));
		}

		template<typename Ret, typename ...Args, std::size_t ...Indexes>
		typename std::enable_if<sizeof...(Args) >= 2, void>::type
			regist_impl(const std::string &funcname, std::function<Ret(Args...)> &&func, std::index_sequence<Indexes...>)
		{
			auto func_impl = [func](uint8_t *&ptr, uint8_t *const end) ->std::string
			{
				auto tp= detail::endec::get<typename std::remove_const<typename std::remove_reference<Args>::type>::type...>(ptr, end);
				Ret result = func(std::get<Indexes>(tp)...);
				std::string buffer;
				buffer.resize(detail::endec::get_sizeof(result));
				auto buffer_ptr = (uint8_t *)(buffer.data());
				detail::endec::put(buffer_ptr, result);
				return std::move(buffer);
			};
			std::lock_guard<mutex> locker(mtex_);
			functions_.emplace(funcname, std::move(func_impl));
		}
		template<typename Ret, typename Arg>
		void regist_impl(const std::string &funcname, std::function<Ret(Arg)> &&func)
		{
			auto func_impl = [func](uint8_t *&ptr, uint8_t *const end) ->std::string
			{
				using type = typename std::remove_const<typename std::remove_reference<Arg>::type>::type;
				Ret result = func(detail::endec::get<type>(ptr, end));
				std::string buffer;
				buffer.resize(detail::endec::get_sizeof(result));
				auto buffer_ptr = (uint8_t *)(buffer.data());
				detail::endec::put(buffer_ptr, result);
				return std::move(buffer);
				return{};
			};
			std::lock_guard<mutex> locker(mtex_);
			functions_.emplace(funcname, std::move(func_impl));
		}
		std::function<std::string(uint8_t *&, uint8_t *const)> &get_function(const std::string &funcname)
		{
			std::lock_guard<mutex> locker(mtex_);
			auto itr = functions_.find(funcname);
			if (itr == functions_.end())
				throw std::runtime_error("not find function with name:" + funcname);
			return itr->second;
		}
		mutex mtex_;
		std::map<std::string, std::function<std::string(uint8_t *&, uint8_t *const)>> functions_;
	};
}
