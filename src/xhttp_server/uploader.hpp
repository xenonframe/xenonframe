#pragma once
namespace xhttp_server
{
	class uploader
	{
	public:
		uploader(request &_request)
			:request_(_request)
		{
			init();
		}
		~uploader()
		{
			request_.do_receive();
		}
		bool parse_request(const std::string &path = "")
		{
			path_ = path;
			content_length_ = request_.content_length();
			if (!content_length_)
				return false;
			if (!check_boundary())
				return  false;
			return do_parse();
		}
		std::map<std::string, std::string> get_field()
		{
			return fields_;
		}
		std::map<std::string, std::string> get_files()
		{
			return files_;
		}
	private:
		void init()
		{
			mime_parser_.regist_header_callback(
				[&](const std::string &header_name,const std::string &header_value){
				mime_header_callback(header_name, header_value);
			});
			mime_parser_.regist_data_callback([&](std::string &&data){
				data_callback(std::move(data));
			});
			mime_parser_.regist_end_callback([&] {
				close_file();
			});
		}
		bool check_boundary()
		{
			auto boundary = request_.get_boundary();
			if (boundary.empty())
				return false;
			mime_parser_.set_boundary(boundary);
			return true;
		}
		bool do_parse()
		{
			auto buffer = request_.parser_.get_string();
			char *ptr = (char*)buffer.data();
			std::size_t length = buffer.length();
			if (mime_parser_.do_parse(ptr, length))
				return true;
			std::function<void()> resume_handle;
			xnet::connection &conn = request_.conn_;
			conn.regist_recv_callback([&](char *data, std::size_t len) {
				if (len == 0)
				{
					result_ = false;
					conn.close();
					request_.close();
					resume_handle();
				}
				if (mime_parser_.do_parse(data, len))
				{
					resume_handle();
					return;
				}
				conn.async_recv_some();
			});
			conn.async_recv_some();
			xcoroutine::yield(resume_handle);
			return true;
		}
		void mime_header_callback(const std::string &name, 
			const std::string &value)
		{
			close_file();
			if (name == "Content-Disposition")
			{
				auto beg = value.find("name=\"");
				if (beg == std::string::npos)
					return;
				beg += strlen("name=\"");
				auto end = value.find("\"",beg,1);
				if (end == std::string::npos)
					return;
				header_name_ = value.substr(beg, end - beg);
				auto filepath_pos = value.find("filename=\"");
				if (filepath_pos  == std::string::npos)
					is_form_field_ = true;
				else
				{
					is_form_field_ = false;
					filepath_pos += strlen("filename=\"");
					std::string filepath = 
						value.substr(filepath_pos, value.size() - filepath_pos - 1);
					if (filepath.empty())
					{
						filename_.clear();
						return;
					}
					auto filename_pos = filepath.find_last_of('/');
					if(filename_pos == std::string::npos)
						filename_pos = filepath.find_last_of('\\');
					if (filename_pos == std::string::npos)
					{
						filename_ = filepath;
					}
					else
					{
						filename_pos += 1;
						filename_ = filepath.substr(filename_pos,
							filepath.size() - filename_pos);
					}
					files_.emplace(std::move(header_name_), path_ + filename_);
				}
			}
		}
		void data_callback(std::string &&data)
		{
			if (is_form_field_)
			{
				fields_.emplace(std::move(header_name_), std::move(data));
			}
			else if(filename_.size())
			{
				if (!file_.is_open())
				{
					file_.open(path_ + filename_, 
						std::ios::binary | std::ios::trunc);
					assert(file_.is_open());
				}
				file_.write(data.data(), data.size());
				data.clear();
			}
		}
		void close_file()
		{
			if (file_.is_open())
				file_.close();
		}
		std::ofstream file_;

		bool is_form_field_ = false;
		std::string filename_;
		std::string header_name_;
		xhttper::mime_parser mime_parser_;
		std::size_t content_length_ = 0;
		bool result_ = false;
		std::string path_;
		std::map<std::string, std::string> fields_;
		std::map<std::string, std::string> files_;
		request &request_;
	};
}