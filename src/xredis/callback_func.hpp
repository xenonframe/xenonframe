#pragma once
namespace xredis
{
	template<typename T>
	struct callback_func
	{
		typedef std::function<void(std::string &&, T&&)> callback_handle;
	};

	using cluster_slots_callback = typename callback_func<cluster_slots >::callback_handle;

	using string_map_callback =  typename callback_func<std::map<std::string, std::string>>::callback_handle;

	using array_string_callback = typename callback_func<std::list<std::string>>::callback_handle;

	using double_map_callback = typename callback_func<std::map<std::string, double>>::callback_handle;

	using string_callback = typename callback_func<std::string>::callback_handle;

	using integral_callback = typename callback_func<uint32_t>::callback_handle;

	using cluster_init_callback = typename callback_func<bool>::callback_handle;
}