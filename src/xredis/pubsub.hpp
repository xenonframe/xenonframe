#pragma once
#include "xredis.hpp"
namespace xredis
{
	
	class subpub
	{
	public:
		subpub(redis &_redis)
			:redis_(_redis)
		{
		}
		void psubscribe(const std::vector<std::string> &topics,
						std::function<void(bool, pmessage &&)> &&callback)
		{

		}
	private:
		redis &redis_;
	};
}