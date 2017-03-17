#include <string>
#include <iostream>
#include <functional>
#include <type_traits>
#include <vector>
#include "xcoroutine/xcoroutine.hpp"

struct user
{
	user() {}
	user(int id)
		:id_(id) 
	{}
	int id_;
};

std::vector<std::function<void()>> callback0_;
std::vector<std::function<void(std::string)>>callback1;
std::vector<std::function<void(std::string &&, int)>> callback2;
std::vector<std::function<void(std::string &&, int, bool, user&&)>> callbackN;


void async_do0(std::function<void()> callback)
{
//	throw std::exception();
	callback0_.push_back(callback);
}
void async_do1(const std::string &str, std::function<void(std::string)> callback)
{
	callback1.push_back(callback);
}
void async_do2(const std::string &str, std::function<void(std::string &&, int)> callback)
{
	callback2.push_back(callback);
}
void async_doN(const std::string &str, std::string && data, std::function<void(std::string &&, int, bool, user&&)> callback)
{
	callbackN.push_back(callback);
}
int id = 1;
void xroutine_func2()
{
	int i = id++;
	using xutil::to_function;
	using xcoroutine::apply;
	
	try
	{
		apply(to_function(async_do0));
	}
	catch (const std::exception&)
	{
		std::cout << "catch exception" << std::endl;
	}

	auto res = apply(to_function(async_do1),"hello");
	assert(std::get<0>(res) == "hello world");

	auto res2 = apply(to_function(async_do2), "hello");
	assert(std::get<0>(res2) == "hello world");
	assert(std::get<1>(res2) == 1);

	auto res3 = apply(to_function(async_doN), "hello", "hello2");
	assert(std::get<0>(res3) == "hello world");
	assert(std::get<1>(res3) == 1);
	assert(std::get<2>(res3) == true);
	assert(std::get<3>(res3).id_ == 1);

}
int main()
{
	xcoroutine::create(xroutine_func2);
	xcoroutine::create(xroutine_func2);
	xcoroutine::create(xroutine_func2);
	xcoroutine::create(xroutine_func2);
	xcoroutine::create(xroutine_func2);

	for(auto &itr: callback0_)
		itr();
	for(auto &itr: callback1)
		itr("hello world");
	for(auto &itr: callback2)
		itr("hello world", 1);
	for(auto &itr: callbackN)
		itr("hello world", 1, true, user(1));
	return 0;
}
