#include <type_traits>
#include <vector>
#include "xsimple_rpc/xsimple_rpc.hpp"
#include "xtest/xtest.hpp"

XTEST_MAIN;

struct MyStruct
{
	std::string hello;
	int world;
	std::vector<int> ints;

	XENDEC(hello, world, ints);

	std::string func(int, int)
	{
		return hello;
	}

	std::string func2()
	{
		return hello;
	}
	void func3()const
	{
		return;
	}
};

using namespace xsimple_rpc::detail;

namespace funcs
{
	DEFINE_RPC_PROTO(hello, void(int, bool, const MyStruct&))
}


XTEST_SUITE(endnc)
{
	XUNIT_TEST(hello)
	{
		int i = 0;
		std::pair<int,int> ret = std::make_pair(i++,++i);

		std::cout << ret.first <<": "<< ret.second << std::endl;
	}
	XUNIT_TEST(decode)
	{
		MyStruct obj, ob2;
		obj.hello = "hello";
		obj.world = 192982772;
		obj.ints = { 1,2,3,4,5 };

		std::string buffer;
		buffer.resize(obj.xget_sizeof());
		uint8_t *ptr = (uint8_t *)buffer.data();
		obj.xencode(ptr);

		ptr = (uint8_t *)buffer.data();
		ob2.xdecode(ptr, ptr + buffer.size());

		xassert(obj.hello== ob2.hello);
		xassert(obj.world == ob2.world);
		xassert(obj.ints == ob2.ints);
	}

	XUNIT_TEST(map_put)
	{
		std::map<std::string, MyStruct> map;
		std::string buffer;
		buffer.resize(endec::get_sizeof(map));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, map);
	}

	XUNIT_TEST(map_get)
	{
		std::map<std::string, MyStruct> map;
		std::string buffer;
		buffer.resize(endec::get_sizeof(map));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, map);

		ptr = (uint8_t *)buffer.data();
		auto res = endec::get<decltype(map)>(ptr, ptr + buffer.size());
	}

	XUNIT_TEST(set_put)
	{
		std::set<std::string> set;

		std::string buffer;
		buffer.resize(endec::get_sizeof(set));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, set);
	}

	XUNIT_TEST(set_get)
	{
		std::set<std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);

		ptr = (uint8_t *)buffer.data();
		auto res = endec::get<decltype(obj)>(ptr, ptr+ buffer.size());
	}

	XUNIT_TEST(list_put)
	{
		std::list<std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);
	}

	XUNIT_TEST(list_get)
	{
		std::list<std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);

		ptr = (uint8_t *)buffer.data();
		auto res = endec::get<decltype(obj)>(ptr, ptr + buffer.size());
	}

	XUNIT_TEST(pair_put)
	{
		std::pair<std::string, std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);
	}

	XUNIT_TEST(pair_get)
	{
		std::pair<std::string, std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);

		ptr = (uint8_t *)buffer.data();
		auto res = endec::get<decltype(obj)>(ptr, ptr + buffer.size());
	}
}
XTEST_SUITE(rpc_server)
{
	XUNIT_TEST(regist)
	{
		using namespace xsimple_rpc;

		MyStruct obj;
		obj.hello = "hello";
		obj.world = 192982772;
		obj.ints = { 1,2,3,4,5 };
		rpc_proactor_pool rpc_proactor_pool_;
		rpc_server rpc_server_(rpc_proactor_pool_);
		rpc_server_.regist("hello world", [](int)->std::string { return{}; });
		rpc_server_.regist("func", &MyStruct::func, obj);
		rpc_server_.regist("func2", &MyStruct::func2, obj);
		rpc_server_.regist("func3", &MyStruct::func3, obj);
		rpc_server_.regist("get_mystruct", [&] {return obj; });
	}
}
XTEST_SUITE(async_client)
{
}
