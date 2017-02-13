#pragma once
namespace xredis
{
	namespace detail
	{
		struct cmd_builder
		{
			std::string operator()(std::vector<std::string> &&chunks)
			{
				std::string buf;
				buf.reserve(1024);
				buf.push_back('*');
				buf.append(std::to_string(chunks.size()));
				buf.append("\r\n");
				for (auto &itr : chunks)
				{
					buf.push_back('$');
					buf.append(std::to_string(itr.size()));
					buf.append("\r\n");
					buf.append(itr);
					buf.append("\r\n");
				}
				return buf;
			}
		};
	}
}