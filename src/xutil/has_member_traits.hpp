#pragma  once
#define HAS_MEMBER(member)\
template<typename T, typename... Args>struct has_member_##member\
{\
	private:\
		template<typename U> static auto Check(int) -> \
			decltype(std::declval<U>().member(std::declval<Args>()...), std::true_type()); \
\
		template<typename U> static std::false_type Check(...);\
	public:\
	       	enum{value = std::is_same<decltype(Check<T>(0)), std::true_type>::value};\
};
