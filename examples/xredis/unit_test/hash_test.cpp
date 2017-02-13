#include "../include/hash.hpp"


template<typename ...T>
int hello(const std::string &key, T && ...value, const std::string &str)
{

}

XTEST_SUITE(hash)
{
	xnet::proactor pro;
	xredis::redis redis(pro);


	void hmget_test()
	{
		xredis::cmd::hash hash(redis);
		hash.hmget("hash", { "hello2" } , [](std::string && status, std::map<std::string, std::string>&& result)
		{
			if (status.size())
			{
				std::cout << status << std::endl;
				return;
			}
		});

		
	}
	void hset_test()
	{
		xredis::cmd::hash hash(redis);
		hash.hset("hash", "hello", "world", [](std::string && status, uint32_t && result) {
			if (status.size())
			{
				std::cout << status << std::endl;
				return;
			}
			std::cout << result << std::endl;
		});
	}

	void hget_test()
	{
		xredis::cmd::hash hash(redis);
		hash.hget("hash", "hello", [](std::string && status, std::string && result) {
			if (status.size())
			{
				std::cout << status << std::endl;
				return;
			}
			std::cout << result << std::endl;
		});
	}

	void hgetall_test()
	{
		xredis::cmd::hash hash(redis);
		hash.hgetall("hash", [](std::string && status, std::map<std::string, std::string> &&string_map) {
			if (status.size())
			{
				std::cout << status << std::endl;
				return;
			}
		});
	}
	XUNIT_TEST(set_addr)
	{
		redis.set_addr("192.168.3.224",7000);
		redis.regist_cluster_init_callback([](std::string &&, bool result) {
			if (!result)
				xassert(false);
			hset_test();
			hget_test();
		});
	}
	
	
	XUNIT_TEST(run)
	{
		pro.run();
	}
}
