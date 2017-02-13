#pragma once
namespace xhttp_client
{
	class response
	{
	public:
		response(xnet::connection & conn)
			:conn_(conn)
		{
			conn_.regist_recv_callback([&](char *data, std::size_t len) {
				if (len == 0)
				{
					close();
					return;
				}
				http_parser_.append(data, len);
				if (http_parser_.parse_rsp())
				{
					parser_done_callback_();
					return;
				}
				conn_.async_recv_some();
			});
		}
		int get_status()
		{
			return std::strtol(http_parser_.get_status().c_str(), nullptr, 10);
		}
		std::string get_status_str()
		{
			return http_parser_.get_status_str();
		}
		std::string get_entry(const std::string &name)
		{
			using xutil::functional::strncasecmper;

			return http_parser_.get_header<strncasecmper>(name.c_str());
		}
		std::size_t get_content_length()
		{
			if (content_length_ == (size_t)-1)
			{
				auto buf = get_entry("Content-Length");
				if (buf.empty())
					content_length_ = 0;
				content_length_ = std::strtoul(buf.c_str(), 0, 10);
			}
			return content_length_;
		}
		bool check_chunked() noexcept
		{
			if (is_chunked_ == INT32_MAX)
			{
				std::string buffer = get_entry("Transfer-Encoding");
				if (buffer.empty())
				{
					is_chunked_ = 0;
					return false;
				}
				if (xutil::functional::strcasecmper()(buffer.c_str(), "chunked"))
				{
					is_chunked_ = 1;
					return true;
				}
			}
			return !!is_chunked_;
			
		}
		std::string get_body() throw()
		{
			auto crlf_len = strlen("\r\n");
			auto end_chunked_len = strlen("0\r\n\r\n");
			if(!get_string_from_parser_)
				body_buffer_ = http_parser_.get_string();
			get_string_from_parser_ = true;
			if (check_chunked())
			{
				while(!check_chunked_head(body_buffer_))
					async_recv_data(0);
				auto len = std::strtoul(body_buffer_.c_str(), &body_buffer_pos_, 16) + crlf_len;
				body_buffer_pos_ += crlf_len;
				if (len == crlf_len)
				{
					auto remain_len = end_chunked_len - body_buffer_.size();
					if (remain_len > 0)
						async_recv_data(remain_len);
					return{};
				}
				return_data:
				std::size_t remain_len = body_buffer_.size() - (body_buffer_pos_ - body_buffer_.c_str());
				if (remain_len >= len)
				{
					std::string body(body_buffer_pos_, len - crlf_len);
					body_buffer_pos_ += len;
					auto offset = body_buffer_pos_ - body_buffer_.c_str();
					auto body_remain = body_buffer_.size() - offset;
					body_buffer_ = body_buffer_.substr(offset, body_remain);
					return body;
				}
				async_recv_data(len - remain_len);
				goto return_data;
			}
			if (body_buffer_.size() == get_content_length())
				return std::move(body_buffer_);
			async_recv_data(get_content_length() - body_buffer_.size());
			return std::move(body_buffer_);
		}
	private:
		bool check_chunked_head(const std::string &chunked_data)
		{
			return chunked_data.find("\r\n") != chunked_data.npos;
		}
		void async_recv_data(std::size_t to_reads)
		{
			std::function<void()> done;
			conn_.regist_recv_callback([&](char *data, std::size_t len) {

				if (len == 0)
				{
					close();
					return;
				}
				body_buffer_.append(data, len);
				done();
			});
			if (to_reads)
			{
				conn_.async_recv(to_reads);
			}
			else
			{
				conn_.async_recv_some();
			}
			xcoroutine::yield(done);
			if (is_close_)
				throw std::logic_error("lost connection");
		}
		void reset()
		{
			http_parser_.reset();
			content_length_ = (size_t)-1;
		}
		void close()
		{
			is_close_ = true;
		}

		friend class client;
		bool get_string_from_parser_ = false;
		bool is_close_ = false;
		std::string body_buffer_;
		char *body_buffer_pos_ = nullptr;
		std::size_t content_length_ = (size_t)-1;
		std::function<void()> parser_done_callback_;
		xhttper::http_parser http_parser_;
		xnet::connection &conn_;
		int is_chunked_ = INT32_MAX;
	};
}