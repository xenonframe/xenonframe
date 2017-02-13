#pragma once
#include <type_traits>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

namespace xsimple_rpc
{
	namespace detail
	{

		namespace endec
		{
			HAS_MEMBER(xencode);
			HAS_MEMBER(xdecode);
			HAS_MEMBER(xget_sizeof);

			template <typename>
			struct _Is_string :std::false_type
			{
			};

			template<>
			struct _Is_string<std::string> :std::true_type
			{
			};
			template<typename T>
			struct is_string :_Is_string<typename std::remove_const<typename std::remove_reference<T>::type>::type>
			{

			};

			template<typename T>
			struct remove_const_ref
			{
				using type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
			};

			template<typename T>
			inline typename std::enable_if<std::is_arithmetic<T>::value, uint8_t>::type
				get_type(T)
			{
				return (uint8_t)sizeof(T);
			}

			template <typename T>
			inline typename std::enable_if<std::is_arithmetic<T>::value, std::size_t>::type
				get_sizeof(T)
			{
				return sizeof(T);
			}

			template<typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
			inline std::size_t get_sizeof(T)
			{
				return sizeof(typename std::underlying_type<T>::type);
			}

			template <typename T>
			inline typename std::enable_if<has_member_xget_sizeof<T>::value, std::size_t>::type
				get_sizeof(const T& value)
			{
				return value.xget_sizeof();
			}

			inline std::size_t get_sizeof(const std::string &value)
			{
				return sizeof(uint32_t) + value.size();
			}

			template<typename T>
			inline std::size_t get_sizeof(const std::vector<T> &value)
			{
				auto size = sizeof(uint32_t);
				for (auto &itr : value)
					size += get_sizeof(itr);
				return size;
			}

			template<typename T>
			inline std::size_t get_sizeof(const std::list<T> &value)
			{
				auto size = sizeof(uint32_t);
				for (auto &itr : value)
					size += get_sizeof(itr);
				return size;
			}
			template<typename first_type, typename second_type>
			inline std::size_t get_sizeof(const std::pair<first_type, second_type> &value)
			{
				return get_sizeof(value.first) + get_sizeof(value.second);
			}

			template<typename key_type, typename map_type>
			inline std::size_t get_sizeof(const std::map<key_type, map_type> &value)
			{
				auto size = sizeof(uint32_t);
				for (auto &itr : value)
					size += get_sizeof(itr);
				return size;
			}

			template<typename key_type>
			inline std::size_t get_sizeof(const std::set<key_type> &value)
			{
				auto size = sizeof(uint32_t);
				for (auto &itr : value)
					size += get_sizeof(itr);
				return size;
			}

			template<typename First, typename ...Rest>
			inline std::size_t get_sizeof(const First &first, const Rest &...rest)
			{
				return get_sizeof(first) + get_sizeof(rest...);
			}

			template<typename ...Args, std::size_t ...Indexes>
			inline std::size_t get_sizeof(const std::tuple<Args...> &tp, std::index_sequence<Indexes...>)
			{
				return get_sizeof(std::get<Indexes>(tp)...);
			}
			template<typename ...Args>
			inline std::size_t get_sizeof(const std::tuple<Args...> &tp)
			{
				return get_sizeof(tp, std::make_index_sequence<sizeof...(Args)>());
			}


			template<typename T>
			inline typename std::enable_if<has_member_xdecode<T, uint8_t *&, uint8_t *const>::value, T>::type
			       	get(uint8_t *&ptr, uint8_t *const end)
			{
				T value;
				value.xdecode(ptr, end);
				return value;
			}

			template<typename T>
			inline  typename std::enable_if<has_member_xencode<T, uint8_t *&>::value, void>::type
		       		put(uint8_t *&ptr, const T &value)
			{
				value.xencode(ptr);
			}

			//bool 
			template<typename T>
			inline typename std::enable_if<std::is_same<T, bool>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				uint8_t value = ptr[0];
				ptr += sizeof(value);
				return value > 0;
			}
			template<typename T>
			inline typename std::enable_if<std::is_same<T, bool>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				*ptr = value ? 1 : 0;
				ptr += sizeof(char);
			}

			//integral
			template<typename T>
			inline typename std::enable_if<std::is_same<T, uint8_t>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				uint8_t value = ptr[0];
				ptr += sizeof(value);
				return value;
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, int8_t>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				return static_cast<T>(get<uint8_t>(ptr, end));
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, uint8_t>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				*ptr = value;
				ptr += sizeof(value);
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, int8_t>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				return put(ptr, (uint8_t)value);
			}


			template<typename T>
			inline typename std::enable_if<std::is_same<T, uint16_t>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				uint16_t value =
					(((uint16_t)ptr[0]) << 8) |
					((uint16_t)ptr[1]);
				ptr += sizeof(value);
				return value;
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, uint16_t>::value, void>::type
				put(uint8_t *ptr, T value)
			{
				ptr[0] = (unsigned char)(((value) >> 8) & 0xff);
				ptr[1] = (unsigned char)(value & 0xff);
				ptr += sizeof(value);
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, int16_t>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				return static_cast<T>(get<uint16_t>(ptr, end));
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, int16_t>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				return put(ptr, (uint16_t)value);
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, uint32_t>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				uint32_t value =
					(((uint32_t)ptr[0]) << 24) |
					(((uint32_t)ptr[1]) << 16) |
					(((uint32_t)ptr[2]) << 8) |
					((uint32_t)ptr[3]);
				ptr += sizeof(value);
				return value;
			}


			template<typename T>
			inline typename std::enable_if<std::is_same<T, uint32_t>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				ptr[0] = (unsigned char)(((value) >> 24) & 0xff);
				ptr[1] = (unsigned char)(((value) >> 16) & 0xff);
				ptr[2] = (unsigned char)(((value) >> 8) & 0xff);
				ptr[3] = (unsigned char)(value & 0xff);
				ptr += sizeof(value);
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, int32_t>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				return static_cast<T>(get<uint32_t>(ptr, end));
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, int32_t>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				return put(ptr, uint32_t(value));
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, uint64_t>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				uint64_t value =
					((((uint64_t)ptr[0]) << 56) |
					(((uint64_t)ptr[1]) << 48) |
						(((uint64_t)ptr[2]) << 40) |
						(((uint64_t)ptr[3]) << 32) |
						(((uint64_t)ptr[4]) << 24) |
						(((uint64_t)ptr[5]) << 16) |
						(((uint64_t)ptr[6]) << 8) |
						((uint64_t)ptr[7]));
				ptr += sizeof(value);
				return value;
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, uint64_t>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				ptr[0] = (unsigned char)(((value) >> 56) & 0xff);
				ptr[1] = (unsigned char)(((value) >> 48) & 0xff);
				ptr[2] = (unsigned char)(((value) >> 40) & 0xff);
				ptr[3] = (unsigned char)(((value) >> 32) & 0xff);
				ptr[4] = (unsigned char)(((value) >> 24) & 0xff);
				ptr[5] = (unsigned char)(((value) >> 16) & 0xff);
				ptr[6] = (unsigned char)(((value) >> 8) & 0xff);
				ptr[7] = (unsigned char)(value & 0xff);
				ptr += sizeof(value);
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, int64_t>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("endec::get out_of_range");
				return static_cast<T>(get<uint64_t>(ptr, end));
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, int64_t>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				return put(ptr, uint64_t(value));
			}

			template<typename T, typename std::enable_if<std::is_enum<T>::value, void>::type* = nullptr>
			void put(uint8_t *&ptr, T value)
			{
				put(ptr, static_cast<typename std::underlying_type<T>::type>(value));
			}

			template<typename T, typename std::enable_if<std::is_enum<T>::value, void>::type* = nullptr>
			auto get(uint8_t *&ptr, uint8_t *const end)
			{
				return static_cast<T>(get<typename std::underlying_type<T>::type>(ptr, end));
			}

			template<typename T>
			inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				if (end - ptr < (int)sizeof(T))
					throw std::out_of_range("");
				auto value = *reinterpret_cast<T*>(ptr);
				ptr += sizeof(value);
				return value;
			}
			template<typename T>
			inline typename std::enable_if<std::is_floating_point<T>::value, void>::type
				put(unsigned char *&ptr, T value)
			{
				T &ref = *reinterpret_cast<T*>(ptr);
				ref = value;
				ptr += sizeof(value);
			}

			//string 
			template<typename T>
			inline typename std::enable_if<std::is_same<T, std::string>::value, T>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				auto len = get<uint32_t>(ptr, end);
				if (end - ptr < (int)len)
					throw std::out_of_range("");
				std::string result((char*)ptr, len);
				ptr += len;
				return result;
			}

			template<typename T>
			inline typename std::enable_if<std::is_same<T, std::string>::value, void>::type
				put(uint8_t *&ptr, const T &value)
			{
				put(ptr, (uint32_t)value.size());
				memcpy(ptr, value.data(), value.size());
				ptr += value.size();
			}

			template<typename first_type, typename second_type>
			void put(uint8_t *& ptr, const std::pair<first_type, second_type> &value)
			{
				put(ptr, value.first);
				put(ptr, value.second);
			}

			template<typename Pair,
				typename first_type = typename Pair::first_type, 
				typename second_type = typename Pair::second_type>
			Pair get(uint8_t *& ptr, uint8_t *const end)
			{
				auto first = get<first_type>(ptr, end);
				return { first, get<second_type>(ptr, end)};
			}

			template<typename Container, typename value_type = typename Container::value_type>
			inline typename std::enable_if<!is_string<Container>::value, void>::type
				put(uint8_t *& ptr, const Container &container)
			{
				put(ptr, (uint32_t)container.size());
				for (auto &itr : container)
					put<value_type>(ptr, itr);
			}

			template<typename Container, typename value_type = typename Container::value_type>
			inline typename std::enable_if<std::is_same<Container, std::vector<value_type >>::value  || 
				std::is_same<Container, std::list<value_type >>::value ,Container>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				Container container;
				auto size = get<uint32_t>(ptr, end);
				for (uint32_t i = 0; i < size; ++i)
					container.emplace_back(get<value_type>(ptr, end));
				return container;
			}

			template<typename Container, 
				typename key_type = typename Container::key_type, 
				typename mapped_type = typename Container::mapped_type>
			inline typename std::enable_if<
				std::is_same<std::map<key_type, mapped_type>, Container>::value,
				Container>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				Container container;
				auto size = get<uint32_t>(ptr, end);
				for (uint32_t i = 0; i < size; ++i)
				{
					auto key = get<key_type>(ptr, end);
					container.emplace(key, get<mapped_type>(ptr, end));
				}
				return container;
			}

			template<typename Container,
				typename key_type = typename Container::key_type>
				inline typename std::enable_if<
					std::is_same<std::set<key_type>, Container>::value, 
				Container>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				Container container;
				auto size = get<uint32_t>(ptr, end);
				for (uint32_t i = 0; i < size; ++i)
				{
					container.emplace(get<key_type>(ptr, end));
				}
				return container;
			}

			template<typename Last>
			inline void put_tp_impl(uint8_t *&ptr, const Last& last)
			{
				put(ptr, last);
			}

			template<typename First, typename ... Rest>
			inline void put_tp_impl(uint8_t *&ptr, const First& first, const Rest &...rest)
			{
				put(ptr, first);
				put_tp_impl(ptr, rest...);
			}

			template<std::size_t ... Tndexes, typename ... Args>
			inline void put_tp_helper(uint8_t *&ptr, std::index_sequence<Tndexes...>, const std::tuple<Args...>& tup)
			{
				put_tp_impl(ptr, std::get<Tndexes>(tup)...);
			}

			template<typename ... Args>
			inline void put(uint8_t *&ptr, const std::tuple<Args...>& tup)
			{
				put_tp_helper(ptr, std::make_index_sequence<sizeof...(Args)>(), tup);
			}

			template<typename Last>
			inline std::tuple<Last> get_tp_impl(uint8_t *&ptr,uint8_t *const end, std::index_sequence<0>)
			{
				using value_type = typename std::remove_reference<typename std::remove_const<Last>::type>::type;
				return std::forward_as_tuple(get<value_type>(ptr, end));
			}

			template<typename First, typename ... Rest, std::size_t ... Tndexes>
			inline std::tuple<First, Rest...> get_tp_impl(uint8_t *&ptr,uint8_t *const end, std::index_sequence<Tndexes...>)
			{
				using value_type = typename std::remove_reference<typename std::remove_const<First>::type>::type;
				auto first = get<value_type>(ptr, end);
				return std::tuple_cat(std::forward_as_tuple(std::move(first)), 
					get_tp_impl<Rest...>(ptr, end, std::make_index_sequence<sizeof ...(Rest)>()));
			}

			template<typename ...Args>
			inline std::tuple<Args...> get_tp_helper(uint8_t *&ptr, uint8_t *const end)
			{
				return get_tp_impl<Args...>(ptr, end, std::make_index_sequence<sizeof ...(Args)>());
			}

			template<typename ...Args>
			inline typename std::enable_if<sizeof...(Args) >= 2,std::tuple<Args...>>::type
				get(uint8_t *&ptr, uint8_t *const end)
			{
				return get_tp_helper<Args...>(ptr, end);
			}
		}
	}
}
