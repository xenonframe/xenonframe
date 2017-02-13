#pragma once
#include "xredis.hpp"
namespace xredis
{
	namespace cmd
	{
		class lists
		{
		public:
			lists(redis &_redis)
				:redis_(_redis)
			{

			}
			void blpop(std::vector<std::string> &&keys,int64_t timeout, array_string_callback &&cb)
			{
				std::vector<std::string> vec = { "BLPOP"};
				auto key = keys.front();
				for (auto &itr : keys)
					vec.emplace_back(std::move(itr));
				vec.emplace_back(std::to_string(timeout));
				std::string buf = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(buf), std::move(cb));
			}

			void brpop(std::vector<std::string> &&keys ,int64_t timeout, array_string_callback &&cb)
			{
				std::vector<std::string> vec = { "BRPOP" };
				auto key = keys.front();
				for (auto &itr : keys)
					vec.emplace_back(std::move(itr));
				vec.emplace_back(std::to_string(timeout));
				std::string buf = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(buf), std::move(cb));
			}

			template<typename T>
			void brpoplpush(const std::string &key, T &&source,T &&destination, int64_t timeout, string_callback &&cb)
			{
				std::string buf = cmd_builder()({ 
					"BRPOPLPUSH",
					key,
					std::forward<T>(source), 
					std::forward<T>(destination) ,
					std::to_string(timeout)});
				redis_.req(key, std::move(buf), std::move(cb));
			}

			void lindex(const std::string &key, int64_t index, string_callback &&cb)
			{
				std::string buf = cmd_builder()({ "LINDEX", key , std::to_string(index)});
				redis_.req(key, std::move(buf), std::move(cb));
			}

			template<typename T>
			void linsert(const std::string &key, bool before, T && pivot, T &&value, integral_callback &&cb)
			{
				std::string position = "AFTER";
				if (before)
					position = "BEFORE";

				std::string buf = cmd_builder()({
					"LINSERT", 
					key , 
					std::move(position), 
					std::forward<T>(pivot), 
					std::forward<T>(value)});

				redis_.req(key, std::move(buf), std::move(cb));
			}

			void llen(const std::string &key, integral_callback &&cb)
			{
				std::string buf = cmd_builder()({ "LLEN", key });
				redis_.req(key, std::move(buf), std::move(cb));
			}

			void lpop(const std::string &key, string_callback &&cb)
			{
				std::string buf = cmd_builder()({ "LPOP", key});
				redis_.req(key, std::move(buf), std::move(cb));
			}

			void lpush(const std::string &key,std::vector<std::string> &&values,integral_callback &&cb)
			{
				std::vector<std::string> vec = { "LPUSH", key };
				for (auto &itr : values)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void lpushx(const std::string &key, std::vector<std::string> &&values, integral_callback &&cb)
			{
				std::vector<std::string> vec = {"LPUSHX", key };
				for (auto &itr : values)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void lrange(const std::string &key,int64_t start,int64_t stop,array_string_callback &&cb)
			{
				std::string buf = cmd_builder()({ 
					"LRANGE", 
					key, 
					std::to_string(start), 
					std::to_string(stop) });
				redis_.req(key, std::move(buf), std::move(cb));
			}

			template<typename T>
			void lrem(const std::string &key,int64_t count, T &&value,integral_callback &&cb)
			{
				std::string buf = cmd_builder()({
					"LREM", 
					key, 
					std::to_string(count), 
					std::forward<T>(value)});
				redis_.req(key, std::move(buf), std::move(cb));
			}

			template<typename T>
			void lset(const std::string &key,int64_t index, T &&value,string_callback &&cb)
			{
				std::string buf = cmd_builder()({ 
					"LSET", 
					key,
					std::to_string(index),
					std::forward<T>(value) });
				redis_.req(key, std::move(buf), std::move(cb));
			}

			template<typename T>
			void ltrim(const std::string &key, int64_t start, int64_t stop, string_callback &&cb)
			{
				std::string buf = cmd_builder()({ 
					"LTRIM", 
					key, 
					std::to_string(start), 
					std::to_string(stop) });
				redis_.req(key, std::move(buf), std::move(cb));
			}

			void rpop(const std::string &key, string_callback &&cb)
			{
				std::string buf = cmd_builder()({ "RPOP", key});
				redis_.req(key, std::move(buf), std::move(cb));
			}

			template<typename T>
			void rpoplpush(const std::string &key, T &&source, T &&destination, string_callback &&cb)
			{
				std::string buf = cmd_builder()({
					"RPOPLPUSH", 
					key, 
					std::forward<T>(source), 
					std::forward<T>(destination)});
				redis_.req(key, std::move(buf), std::move(cb));
			}

			void rpush(const std::string &key, std::vector<std::string> &&values,integral_callback &&cb)
			{
				std::vector<std::string> vec = { "RPUSH", key };
				for (auto &itr : values)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

			void rpushx(const std::string &key, std::vector<std::string> &&values, integral_callback &&cb)
			{
				std::vector<std::string> vec = { "RPUSHX", key };
				for (auto &itr : values)
					vec.emplace_back(std::move(itr));
				std::string data = cmd_builder()(std::move(vec));
				redis_.req(key, std::move(data), std::move(cb));
			}

		private:
			redis &redis_;
		};
	}
	
}
