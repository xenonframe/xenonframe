#include "../../xtest/include/xtest.hpp"
#include "../include/xjson.hpp"

xtest_run;

using namespace xjson;
XTEST_SUITE(xjson)
{
	XUNIT_TEST(get)
	{
		obj_t o(int64_t(1));
		xassert(o.get<int>() == 1);
	}
	struct oo
	{
		int id;
		int b;

		XGSON(id, b);
	};
	struct user
	{
		int id;
		std::string name;
		std::vector<int> ints;
		std::map<std::string, int> int_map_;
		std::map<std::string, oo> oos;
		XGSON(id, name, ints, int_map_, oos);
	};


	XUNIT_TEST(XGSON)
	{
		user u{ 1,"u1" };

		obj_t o;
		o = u;
		auto u0 =  o.get<user>();

		std::cout << o.str() << std::endl;

		o["user"] = u;
		o["users"].add(u);

		std::cout << o.str() << std::endl;

		xassert(o["user"]["id"].get<int>() == 1);
		xassert(o["user"]["name"].get<std::string>() == "u1");

		auto u2 = o["user"].get<user>();

		xassert(u2.id == u.id);
		xassert(u2.name == u.name);

	}
	XUNIT_TEST(vector)
	{
		obj_t o;

		o["vec"] = std::vector<int>{ 1,2,3,4,5 };

		for (auto &ind : { 1,2,3,4,5 })
		{
			xassert(o["vec"].get<int>(ind - 1) == ind);
		}
		auto vec = o["vec"].get<std::vector<int>>();

		user u{ 1,"u1" };

		o["users"].add(u);
		xassert(o["users"].get<user>(0).id == 1 && o["users"].get<user>(0).name == "u1");
	}

	XUNIT_TEST(map)
	{
		obj_t o;
		o = std::map<std::string, int>{ {"1",1},{ "2",2 } };

		xassert(o["1"].get<int>() == 1);
		xassert(o["2"].get<int>() == 2);

		auto m = o.get<std::map<std::string, int>>();

		o["map"] = std::map<std::string, int>{ { "1",1 },{ "2",2 } };

		xassert(o["map"]["1"].get<int>() == 1);
		xassert(o["map"]["2"].get<int>() == 2);
	}
	XUNIT_TEST(list)
	{

		obj_t o;
		o["vec"] = std::list<int>{ 1,2,3,4,5 };
		o["list_list"] = std::list<std::list<int>>{ {1,2,2,4,5} };

		auto list_list = o["list_list"].get<std::list<std::list<int>>>();

		for (auto &ind : { 1,2,3,4,5 })
		{
			xassert(o["vec"].get<int>(ind - 1) == ind);
		}
		user u{ 1,"u1" };

		o["users"].add(u);
		xassert(o["users"].get<user>(0).id == 1 && o["users"].get<user>(0).name == "u1");
	}
	XUNIT_TEST(optional)
	{
		optional<int> a;
		xassert((!a));
		a = 1;
		xassert( (!!a) );
		xassert(a.get() == 1);

		obj_t o;
		o["a"] = a;
		o["b"] = optional<int>();

		a = o["a"].get<decltype(a)>();

		optional<int> no_exist;
		no_exist = o["no_exist"].get<decltype(a)>();
		xassert((!no_exist));

	}
	XUNIT_TEST(initializer_list)
	{
		obj_t o;
		o["ints"]= { 1,2,3,4,5,6,7 };

		for (auto i : { 1,2,3,4,5,6,7 })
		{
			xassert(o["ints"].get<int>(i - 1) == i);
		}
		obj_t::map_t m = { {"1", 1 },{"2", "2"},{"true", true} } ;
		obj_t o2 = m;
		xassert(o2["1"].get<int>() == 1);
		xassert(o2["2"].get<std::string>() == "2");
		xassert(o2["true"].get<bool>() == true);
	}
}