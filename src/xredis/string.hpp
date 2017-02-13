#pragma once
#include "xredis.hpp"
namespace xredis
{
	namespace cmd
	{
		class string
		{
		public:
			string(redis &_redis)
				:redis_(_redis)
			{

			}

			template<typename T>
			void append(const std::string &key, T &&value, integral_callback &&cb)
			{
				redis_.req(key, cmd_builder()({ "APPEND", key, std::forward<T>(value) }), std::move(cb));
			}

			void bitcount(const std::string &key, int64_t start, int64_t end, integral_callback &&cb)
			{
				auto data = cmd_builder()({"BITCOUNT", key, std::to_string(start), std::to_string(end)});
				redis_.req(key, std::move(data), std::move(cb));
			}

			enum bitop_operation
			{
				AND,
				OR,
				XOR,
				NOT
			};
			void bitop(bitop_operation op, const std::string &destination, std::vector<std::string> &&keys, integral_callback &&cb)
			{
				auto vec = std::vector<std::string>{ "BITOP"};
				std::string op_str;
				switch (op)
				{
				case xredis::cmd::string::AND:
					op_str = "AND";
					break;
				case xredis::cmd::string::OR:
					op_str = "OR";
					break;
				case xredis::cmd::string::XOR:
					op_str = "XOR";
					break;
				case xredis::cmd::string::NOT:
					op_str = "NOT";
					break;
				default:
					break;
				}
				vec.emplace_back(std::move(op_str));
				vec.emplace_back(destination);
				for (auto &itr : keys)
					vec.emplace_back(std::move(itr));
				auto data = cmd_builder()(std::move(vec));
				redis_.req(destination, std::move(data), std::move(cb));
			}

			void mget(std::vector<std::string> &&keys, array_string_callback &&cb)
			{
				std::vector<std::string> vec = {"MGET"};
				auto key = keys.front();
				for (auto &itr : keys)
					vec.emplace_back(std::move(std::move(itr)));
				auto data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void mset(std::vector<std::string> &&key_values, string_callback &&cb)
			{
				std::vector<std::string> vec = { "MSET" };
				auto key = key_values.front();
				for (auto &itr : key_values)
					vec.emplace_back(std::move(std::move(itr)));
				auto data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void msetnx(std::vector<std::string> &&key_values, string_callback &&cb)
			{
				std::vector<std::string> vec = { "MSETNX" };
				auto key = key_values.front();
				for (auto &itr : key_values)
					vec.emplace_back(std::move(std::move(itr)));
				auto data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void psetex(const std::string &key, int64_t milliseconds, const std::string &value, string_callback &&cb)
			{
				auto data = cmd_builder()({"PSETEX", key, std::to_string(milliseconds), value});
				redis_.req(key, std::move(data), std::move(cb));
			}

			void setbit(const std::string &key ,int64_t offset, bool value, integral_callback &&cb)
			{
				auto data = cmd_builder()({ "SETBIT", key, std::to_string(offset), value ? "1" : "0"});
				redis_.req(key, std::move(data), std::move(cb));
			}

			void setex(const std::string &key, int64_t seconds, const std::string &value, string_callback &&cb)
			{
				auto data = cmd_builder()({"SETEX", key, std::to_string(seconds), value});
				redis_.req(key, std::move(data), std::move(cb));
			}

			template<typename T>
			void setnx(const std::string &key, T &&value, integral_callback &&cb)
			{
				redis_.req(key, cmd_builder()({"SETNX", key, std::forward<T>(value)}),std::move(cb));
			}

			void setrange(const std::string &key, int offset ,const std::string &value, integral_callback &&cb)
			{
				redis_.req(key, cmd_builder()({"SETRANGE", std::to_string(offset), value}), std::move(cb));
			}

			void strlen(const std::string &key, integral_callback &&cb)
			{
				redis_.req(key, cmd_builder()({"STRLEN", key}), std::move(cb));
			}
		private:
			redis &redis_;
		};
	}
}