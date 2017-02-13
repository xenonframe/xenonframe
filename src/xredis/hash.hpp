#pragma once
#include "xredis.hpp"
namespace xredis
{
	namespace cmd
	{
		class hash
		{
		public:
			hash(redis &_redis)
				:redis_(_redis)
			{

			}

			template<typename T>
			void hdel(const std::string &key, T&& field, integral_callback &&callback)
			{
				std::string data = cmd_builder()({ "HDEL", key, std::forward<T>(field) });
				redis_.req(key, std::move(data), std::move(callback));
			}

			template<typename T>
			void hexists(const std::string &key, T &&field, integral_callback &&callback)
			{
				std::string data = cmd_builder()({ "HEXISTS", key, std::forward<T>(field) });
				redis_.req(key, std::move(data), std::move(callback));
			}

			template<typename T>
			void hget(const std::string &key, T &&field, string_callback &&callback)
			{
				std::string data = cmd_builder()({ "HGET", key, std::forward<T>(field) });
				redis_.req(key, std::move(data), std::move(callback));
			}

			void hgetall(const std::string &key, string_map_callback &&callback)
			{
				std::string data = cmd_builder()({ "HGETALL", key });
				redis_.req(key, std::move(data), std::move(callback));
			}

			template<typename T>
			void hincrby(const std::string &key, T &&field, int64_t increment, integral_callback &&callback)
			{
				std::string data = cmd_builder()({ "HINCRBY", key, std::forward<T>(field), std::to_string(increment) });
				redis_.req(key, std::move(data), std::move(callback));
			}

			template<typename T>
			void hincrbyfloat(const std::string &key, T &&field, double increment, string_callback &&callback)
			{
				std::string data = cmd_builder()({ "HINCRBYFLOAT", key, std::forward<T>(field), std::to_string(increment) });
				redis_.req(key, std::move(data), std::move(callback));
			}

			void hkeys(const std::string &key, array_string_callback &&callback)
			{
				std::string data = cmd_builder()({ "HKEYS", key });
				redis_.req(key, std::move(data), std::move(callback));
			}

			void hlen(const std::string &key, integral_callback &&callback)
			{
				std::string data = cmd_builder()({ "HLEN", key });
				redis_.req(key, std::move(data), std::move(callback));
			}

			void hmget(const std::string &key, std::vector<std::string>&& fields, string_map_callback &&callback)
			{
				std::vector<std::string> vec = {"HMGET", key};
				for (auto &itr : fields)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(callback));
			}

			void hmset(const std::string &key, std::vector<std::string> &&fields, string_callback &&callback)
			{
				std::vector<std::string> vec = { "HMSET", key };
				for (auto &itr : fields)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(callback));
			}

			template<typename T>
			void hset(const std::string &key, T &&field, T &&value, integral_callback &&callback)
			{
				std::string data = cmd_builder()({ "HSET", key, std::forward<T>(field), std::forward<T>(value) });
				redis_.req(key, std::move(data), std::move(callback));
			}

			template<typename T>
			void hsetnx(const std::string &key, T &&field, T &&value, integral_callback &&callback)
			{
				std::string data = cmd_builder()({ "HSETNX", key, std::forward<T>(field), std::forward<T>(value) });
				redis_.req(key, std::move(data), std::move(callback));
			}

			template<typename T>
			void hstrlen(const std::string &key, T &&field, T &&value, integral_callback &&callback)
			{
				std::string data = cmd_builder()({ "HSTRLEN", key, std::forward<T>(field), std::forward<T>(value) });
				redis_.req(key, std::move(data), std::move(callback));
			}

			void hvals(const std::string &key, array_string_callback &&callback)
			{
				std::string data = cmd_builder()({ "HVALS", key });
				redis_.req(key, std::move(data), std::move(callback));
			}

		private:
			redis &redis_;
		};
	}
}
