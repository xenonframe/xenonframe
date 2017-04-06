#pragma once
namespace xutil
{
	template<int... Is>
	struct index_sequence { };

	template<int N, int... Is>
	struct make_index_sequence : make_index_sequence<N - 1, N - 1, Is...> { };

	template<int... Is>
	struct make_index_sequence<0, Is...> : index_sequence<Is...> { };
}