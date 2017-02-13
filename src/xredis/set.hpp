#pragma once
#pragma once
#include "xredis.hpp"
namespace xredis
{
	namespace cmd
	{
		class set
		{
		public:
			set(redis &_redis)
				:redis_(_redis)
			{
			}
			void sadd(const std::string &key, std::vector<std::string> &&members, integral_callback &&cb)
			{
				std::vector<std::string> vec = { "SADD", key };
				for (auto &itr : members)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void scard(const std::string &key, integral_callback &&cb)
			{
				std::string data = cmd_builder()({ "SCARD", key });
				redis_.req(key, std::move(data), std::move(cb));
			}

			void sdiff(std::vector<std::string> &&keys, array_string_callback &&cb)
			{
				std::vector<std::string> vec = { "SDIFF" };
				auto key = vec.front();
				for (auto& itr : keys)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void sdiffstore(const std::string &destination, std::vector<std::string> &&keys, integral_callback &&cb)
			{
				std::vector<std::string> vec = { "SDIFFSTORE", destination};
				auto key = vec.front();
				for (auto& itr : keys)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void sinter(std::vector<std::string> &&keys, array_string_callback &&cb)
			{
				std::vector<std::string> vec = { "SINTER" };
				auto key = vec.front();
				for (auto& itr : keys)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void sinterstore(const std::string &destination, std::vector<std::string> &&keys, integral_callback &&cb)
			{
				std::vector<std::string> vec = { "SINTERSTORE", destination };
				auto key = vec.front();
				for (auto& itr : keys)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			template<typename T>
			void sismenber(const std::string &key, T &&member, integral_callback &&cb)
			{
				std::string data = cmd_builder()({"SISMEMBER", key, std::forward<T>(member)});
				redis_.req(key, std::move(data), std::move(cb));
			}

			void smembers(const std::string &key, array_string_callback &&cb)
			{
				std::string data = cmd_builder()({ "SMEMBERS", key });
				redis_.req(key, std::move(data), std::move(cb));
			}

			template<typename T>
			void smove(const std::string &source, const std::string& destination, T &&member, integral_callback &&cb)
			{
				std::string data = cmd_builder()({ source, destination, std::forward<T>(member) });
				redis_.req(source, std::move(data), std::move(cb));
			}

			void spop(const std::string &key, int64_t count, array_string_callback &&cb)
			{
				std::string data = cmd_builder()({"SPOP", key, std::to_string(count)});
				redis_.req(key, std::move(data), std::move(cb));
			}

			void srandmember(const std::string &key, string_callback &&cb)
			{
				redis_.req(key, cmd_builder()({ "SRANDMEMBER", key }), std::move(cb));
			}

			void srandmember(const std::string &key, int64_t count, array_string_callback &&cb)
			{
				redis_.req(key, cmd_builder()({ "SRANDMEMBER", key , std::to_string(count)}), std::move(cb));
			}

			void srem(const std::string &key, std::vector<std::string> &&members, integral_callback &&cb)
			{
				std::vector<std::string> vec = {"SREM", key};
				for (auto &itr : members)
					vec.emplace_back(std::move(itr));
				auto data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), cb);
			}

			void sunion(std::vector<std::string> &&keys, array_string_callback &&cb)
			{
				std::vector<std::string> vec = { "SUNION" };
				auto key = vec.front();
				for (auto& itr : keys)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void sunionstore(const std::string &destination, std::vector<std::string> &&keys, integral_callback &&cb)
			{
				std::vector<std::string> vec = { "SUNIONSTORE", destination };
				auto key = vec.front();
				for (auto& itr : keys)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}
		private:
			redis &redis_;
		};
	}
}