#pragma once
#include <tuple>
#include <functional>
#include <utility>

namespace xutil
{
	template<typename>
	struct function_traits;

	template<typename Ret, typename ...Args>
	struct function_traits<Ret(Args...)>
	{
		enum { arity = sizeof...(Args)};
		typedef std::function<Ret(Args...)> stl_function_type;
		using tuple_type = std::tuple<typename std::remove_reference<typename std::remove_const<Args>::type>::type...>;
		
		template<int index>
		struct args
		{
			static_assert(index < arity, "out of function Args's range");
			using type = typename std::tuple_element<index, tuple_type>::type;
		};
	};

	template<typename Ret, typename ...Args>
	struct function_traits<Ret(*)(Args...)> :function_traits<Ret(Args...)> { };

	template<typename Ret, typename Class, typename ...Args>
	struct function_traits<Ret(Class::*)(Args...) const> :function_traits<Ret(Args...)> { };

	template<typename Ret, typename Class, typename ...Args>
	struct function_traits<Ret(Class::*)(Args...)> :function_traits<Ret(Args...)> { };

	template<typename Callable>
	struct function_traits :function_traits<decltype(&Callable::operator())> { };

	template<typename Ret, typename ...Args>
	struct function_traits<std::function<Ret(Args...)>> : function_traits<Ret(Args...)> { };

	template<typename FuncType>
	static inline
		typename xutil::function_traits<FuncType>::stl_function_type
		to_function(const FuncType &func)
	{
		return  typename xutil::function_traits<FuncType>::stl_function_type{ func };
	}
}