#pragma once
#include "detail/detail.hpp"
namespace xhttp_client
{
	class client
	{
	public:
		using response_callback = std::function<void(response &)>;
		client(xnet::connection &&conn)
			:conn_(std::move(conn)),
			response_(conn_)
		{

		}
		~client()
		{
			close();
		}
		client &append_entry(const std::string &name, const std::string &value)
		{
			http_builder_.append_entry(name, value);
			return *this;
		}
		client &append_body(const std::string &data)
		{
			return *this;
		}
		client &append_form(const std::string &name, const std::string &value)
		{
			forms_.emplace_back(name, value);
			return *this;
		}
		client &append_file(const std::string &name, const std::string &filepath)
		{
			files_.emplace_back(name, filepath);
			return *this;
		}
		bool do_get(const std::string &url, const response_callback& handle)
		{
			method_ = "GET";
			url_ = url;
			if (!check()) 
				return false;
			return do_req(handle);
		}
		bool do_post(const std::string &url, response_callback handle)
		{
			method_ = "POST";
			url_ = url;
			if (!check())
				return false;
			return do_req(handle);
		}
	private:
		bool do_req(const response_callback& handle)
		{
			bool result = true;
			conn_.regist_send_callback([&](std::size_t len) {
				if (len == 0)
				{
					close();
					return;
				}
				if (method_ == "GET")
					return;
				is_send_ = false;
				do_upload_data();
			});
			is_send_ = true;
			conn_.async_send(http_builder_.
				set_method(method_).
				set_url(url_).
				build_req());
			conn_.async_recv_some();
			xcoroutine::yield(response_.parser_done_callback_);
			handle(response_);
			reload();
			return true;
		}
		void do_upload_data()
		{
			check_end();
			do_upload_form();
			if (is_send_)
				return;
			do_upload_file();
		}
		void do_upload_file()
		{
			if (file_.eof())
			{
				file_.close();
				std::string buffer("\r\n");
				is_send_ = true;
				conn_.async_send(std::move(buffer));
				return;
			}
			if (files_.empty())
				return;
			if (!file_.is_open())
			{
				file_.open(files_.front().second, std::ios::binary);
				if (!file_.good())
					throw std::runtime_error("open file error: " + files_.front().second);
				std::string buffer = gen_boundary();
				buffer += "\r\n";
				buffer.append("Content-Disposition: form-data; name=\"");
				buffer.append(files_.front().first);
				buffer.append("\"; filename=\""+get_file_name(files_.front().second)+"\"\r\n");
				buffer.append("Content-Type: application/octet-stream\r\n");
				files_.pop_front();
				is_send_ = true;
				conn_.async_send(std::move(buffer));
				return;
			}
			std::string buffer;
			buffer.resize(1024);
			file_.read((char*)buffer.data(), buffer.size());
			is_send_ = true;
			conn_.async_send(buffer.data(), (int)file_.gcount());
		}
		std::string get_file_name(const std::string &filepath)
		{
			auto pos = filepath.find_last_of('/');
			if (pos == std::string::npos)
				pos = filepath.find_last_of('\\');
			if (pos == std::string::npos)
				return filepath;
			++pos;
			return filepath.substr(pos);
		}
		void do_upload_form()
		{
			if (forms_.empty())
				return;
			std::string buffer = gen_boundary();
			buffer.reserve(1024);

			buffer += "\r\n";
			buffer.append("Content-Disposition: form-data; name=\"");
			buffer.append(forms_.front().first);
			buffer.append("\"r\n");
			buffer.append("\"r\n");
			buffer.append(forms_.front().second);
			buffer.append("\"r\n");
			forms_.pop_front();
			is_send_ = true;
			conn_.async_send(std::move(buffer));
		}
		void check_end()
		{
			if (is_end_)
				return;
			if (files_.size() || forms_.size())
				return;
			std::string buffer = gen_boundary();
			buffer += "--\r\n";
			is_send_ = true;
			is_end_ = true;
			conn_.async_send(std::move(buffer));
		}
		bool check()
		{
			if (method_ == "GET")
			{
				if (body_.size() ||
					files_.size() ||
					forms_.size())
					return false;
				return true;
			}
			if (body_.size())
			{
				if (files_.size() || forms_.size())
					return false;
			}
			return true;
		}
		void reload()
		{
			if (reload_)
			{
				reload_ = false;
				http_builder_.reset();
				files_.clear();
				url_.clear();
				body_.clear();
				forms_.clear();
			}
		}
		void close()
		{

		}
		std::string gen_boundary()
		{
			return "----XHttpClientFormBoundaryuwYcfA2AIgxqIxA0";
		}
		std::ifstream file_;
		bool is_end_ = false;
		bool reload_ = false;
		bool is_send_ = false;
		std::string method_;
		xnet::connection conn_;
		response response_;
		xhttper::http_builder http_builder_;
		std::string url_;
		std::string body_;
		std::list<std::pair<std::string, std::string>> files_;
		std::list<std::pair<std::string, std::string>> forms_;
	};
}