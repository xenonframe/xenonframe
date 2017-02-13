#pragma once
#include <fstream>
#include <utility>
namespace xhttp_server
{
	
	class response
	{
	public:
		enum class version
		{
			HTTP_1_0,
			HTTP_1_1
		};
		
		response()
		{

		}
		response &set_version(version v)
		{
			if (v == version::HTTP_1_0)
				builder_.set_version("HTTP/1.0");
			return *this;
		}
		response &set_status(int status)
		{
			builder_.set_status(status);
			if (status == 404 && data_.empty())
				data_ = "404";
			return *this;
		}
		response &set_keep_alive(bool value)
		{
			keep_alive_ = value;
			return *this;
		}
		response &add_entry(const std::string &keyname, const std::string &value)
		{
			builder_.append_entry(keyname, value);
			return *this;
		}
		response &set_content_type(const std::string &content_type)
		{
			content_type_ = content_type;
			return *this;
		}
		bool send_file(const std::string &filepath)
		{
			using get_extension = xutil::functional::get_extension;

			std::ifstream file(filepath.c_str(), std::ios::binary);
			if (!file)
				return false;
			std::string buffer((std::istreambuf_iterator<char>(file)),
				std::istreambuf_iterator<char>());
			file.close();
			data_ = std::move(buffer);
			content_type_ = builder_.get_content_type(get_extension()(filepath));
			return true;
		}
		response &set_date(const std::string &date = xutil::functional::get_rfc1123()())
		{
			date_ = date;
			return *this;
		}
		template<typename T = std::string>
		void done(T &&data = {})
		{
			if(get_size(data))
				data_ = std::forward<T>(data);

			if (date_.empty())
				date_ = xutil::functional::get_rfc1123()();
			builder_.append_entry("Date",std::move(date_));

			if (data_.size())
			{
				builder_.append_entry("Content-type", content_type_);
				builder_.append_entry("Content-Length", std::to_string(data_.size()));
			}

			if (!keep_alive_)
				builder_.append_entry("Connection", "close");
			else
				builder_.append_entry("Connection", "keep-alive");

			std::string buffer = builder_.build_resp();
			buffer.append(data_);
			send_buffer_(std::move(buffer));
			reset();
		}
	private:
		std::size_t get_size(const std::string &str)
		{
			return str.size();
		}

		std::size_t get_size(const char *str)
		{
			if (!str)
				return 0;
			return strlen(str);
		}
		friend class request;
		void reset()
		{
			data_.clear();
			builder_.reset();
		}
		bool keep_alive_ = true;
		std::string date_;
		std::function<void(std::string &&)> send_buffer_;
		std::string data_;
		xhttper::http_builder builder_;
		std::string content_type_ { "text/plain" };
	};
}
