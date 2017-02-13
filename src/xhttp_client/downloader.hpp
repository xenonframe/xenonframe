#pragma once
#include "response.hpp"
namespace xhttp_client
{
	class downloader
	{
	public:
		downloader(response& resp)
			:resp_(resp)
		{

		}
		~downloader()
		{

		}
		downloader &set_path(const std::string &path)
		{
			path_ = path;
			return *this;
		}
		bool do_download(std::vector<std::string> &files)
		{
			return true;
		}
	private:
		std::string path_;
		response &resp_;
	};
}
