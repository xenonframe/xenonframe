#pragma once
namespace xhttp_server
{
	class filelist
	{
	public:
		filelist(response &rep , const std::string &base_path = "")
			:base_path_(base_path),
			resp_(rep)
		{
			if (base_path_.empty())
				base_path_ = xutil::vfs::getcwd()();
		}
		void resp_file_list(const std::string &_path = "")
		{
			auto files = xutil::vfs::ls()(base_path_ + _path);
			if (files.empty())
				return response_404();

			std::string buffer;
			buffer.append("<html>\r\n<body>\r\n<ul>\r\n");
			buffer.append("<li><a href=" + _path + ".."+ "/><h>" + _path + ".."+ "</h></a></li>\r\n");
			for (auto &itr : files)
			{
				if(itr.back() == '/')
					buffer.append("<li><a href=" + _path + itr + "><big>" + _path + itr + "</big></a></li>\r\n");
				else
					buffer.append("<li><a href=" + _path + itr + ">" + _path + itr + "</a></li>\r\n");
			}
			buffer.append("</ul>\r\n</body>\r\n</html>\r\n");
			resp_.set_content_type("text/html");
			resp_.set_status(200);
			resp_.done(std::move(buffer));
		}
	private:
		void response_404()
		{
			resp_.set_status(404);
			resp_.done();
		}

		std::string base_path_;
		response &resp_;
	};
}
