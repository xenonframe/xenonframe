#pragma once
#include <map>
#include <string>
#include <functional>

namespace xactor
{
	struct actor_msg
	{
		std::string msg_name;
		std::string msg_data;
	};
	class actor
	{
	public:
		actor()
		{

		}
		template<typename MSG>
		void regist_msg(const std::function<void(MSG &&)>& func)
		{

		}
	private:
		void handle_msg(const actor_msg &msg)
		{

		}
		std::map<std::string, std::function<void(std::string &)>> msg_handles_;
	};
}