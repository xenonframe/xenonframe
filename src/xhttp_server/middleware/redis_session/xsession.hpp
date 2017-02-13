#pragma once
namespace xhttp_server
{
	class xsession
	{
	public:
		xsession(xredis::redis &redis, const std::string &session_id)
			:redis_(redis),
			session_id_(session_id)
		{

		}
		bool get(const std::string &key, std::string &value)
		{
			bool res = false;
			using xutil::to_function;
			using xcoroutine::apply;
			using namespace std::placeholders;
			xredis::cmd::hash hash(redis_);
			std::function<void(const std::string &,std::string &&, xredis::string_callback&&)> func =
				std::bind(&xredis::cmd::hash::hget<std::string>, std::ref(hash), _1, _2, _3);
			auto result = apply(to_function(func), session_id_, std::string(key));
			if (std::get<0>(result).empty())
					res = true;
			value = std::get<1>(result);
			return res;
		}
		bool set(const std::string &key, std::string &&value)
		{
			bool res = false;
			using xutil::to_function;
			using xcoroutine::apply;
			using namespace std::placeholders;
			xredis::cmd::hash hash(redis_);
			std::function<void(const std::string &, 
				std::string &&, 
				std::string &&, 
				xredis::integral_callback&&)> 
				func = std::bind(&xredis::cmd::hash::hset<std::string>, std::ref(hash), _1, _2, _3, _4);
			auto result = apply(to_function(func), session_id_, std::string(key), std::move(value));
			if (std::get<0>(result).empty())
				res = true;
			return res;
		}
		void del(const std::string &)
		{

		}
	private:
		xredis::redis &redis_;
		std::string session_id_;
	};

	namespace detail
	{
		struct redis_creater
		{
		public:
			static redis_creater &get_instance()
			{
				static redis_creater inst;
				return inst;
			}
			xredis::redis &get_redis(xnet::proactor *proactor = nullptr)
			{
				static thread_local xredis::redis redis(*proactor);
				return redis;
			}

		};


		inline void init_redis_session(
			xnet::proactor_pool &proactor_pool_, 
			const std::string &redis_ip_, 
			int redis_port_, 
			bool redis_cluster_)
		{
			auto &creater = detail::redis_creater::get_instance();
			auto &redis = creater.get_redis(&proactor_pool_.get_current_proactor());

			redis.set_addr(redis_ip_, redis_port_, redis_cluster_);
			if (redis_cluster_)
			{
				redis.regist_cluster_init_callback([](
					std::string &&error_code, bool status) {
					if (error_code.size())
					{
						std::cout << error_code << std::endl;
					}
					else if (status)
					{
						std::cout << "thread: "
							<< std::this_thread::get_id()
							<< " redis cluster init ok"
							<< std::endl;
					}
				});
			}
			else
			{
				redis.regist_connect_success_callback([] {
					std::cout << "thread: "
						<< std::this_thread::get_id()
						<< " redis connect ok" << std::endl;
				});
				redis.regist_connect_failed_callback([](const std::string &error_code) {
					std::cout << "thread: "
						<< std::this_thread::get_id()
						<< " redis connect failed: " << error_code << std::endl;
				});
			}
		}
	}

}
