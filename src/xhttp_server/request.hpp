#pragma once

#include <functional>
#include <chrono>
#include "xutil/functional.hpp"
#include "xnet/connection.hpp"
#include "xcoroutine/xcoroutine.hpp"
#include "xhttp_server/middleware/redis_session/xsession.hpp"
#include "xhttper/query.hpp"
namespace xhttp_server
{
	using namespace xutil::functional;
	class request
	{
	public:
		enum method
		{
			NUll,
			GET,
			POST,
		};
		request()
		{
		}
		method get_method()
		{
			return method_;
		}
		std::size_t content_length()
		{
			if (content_length_ == (std::size_t)-1)
			{
				std::string len = parser_.
					get_header<strncasecmper>("Content-Length");

				if (len.empty())
				{
					content_length_ = 0;
					return content_length_;
				}
				content_length_ = std::strtoul(len.c_str(), 0, 10);
			}
			return content_length_;
		}
		std::string body()
		{
			int i = 0;
			body_ = parser_.get_string();
			if (body_.size() == content_length())
				return body_;
			else
			{
				std::function<void()> resume_handle;
				conn_.regist_recv_callback([&](char *data, std::size_t len)
				{
					if (len == 0) 
					{
						conn_.close();
						resume_handle();
						return;
					}
					i++;
					body_.append(data, len);
					if (body_.size() == content_length())
					{
						resume_handle();
						return;
					}
					conn_.async_recv_some();
				});
				conn_.async_recv_some();
				xcoroutine::yield(resume_handle);
				do_receive();
			}
			return body_;
		}
		bool keepalive()
		{
			if (keepalive_ == -1)
			{
				std::string buffer = parser_.get_header<strncasecmper>("Connection");
				if (buffer.empty())
				{
					keepalive_ = 0;
					return false;
				}
				if (strncasecmper()(buffer.c_str(), "keep-alive", buffer.size()))
				{
					keepalive_ = 1;
				}
			}
			return !!keepalive_;
		}
		std::string get_boundary()
		{
			if (boundary_.size())
				return boundary_;
			std::string content_type = parser_.get_header<strncasecmper>("Content-Type");
			if (content_type.empty())
				return {};
			if (content_type.find("multipart/form-data") == std::string::npos)
				return {};
			auto pos = content_type.find("boundary=");
			if (pos == std::string::npos)
				return {};
			pos += strlen("boundary=");
			boundary_ = content_type.substr(pos, content_type.size() - pos);
			return boundary_;
		}
		xsession get_session()
		{
			std::string session_id = parser_.get_header<strncasecmper>("XSEESSIONID");
			if (session_id.empty())
				session_id = gen_session_id();
			return{ detail::redis_creater::get_instance().get_redis(), session_id };
		}
		std::string get_header(const std::string &name)
		{
			return parser_.get_header<strncasecmper>(name.c_str());
		}
		std::string url()
		{
			return parser_.url();
		}
		std::string path()
		{
			return path_;
		}
		class xserver &get_xserver()
		{
			return *xserver_;
		}
		std::pair<uint64_t, uint64_t> get_range()
		{
			static std::pair<uint64_t, uint64_t>  noexist = { UINT64_MAX, UINT64_MAX };

			std::string range = parser_.get_header<strncasecmper>("Range");
			if (range.empty())
				return noexist;
			auto pos = range.find("=");
			if(pos == range.npos)
				return noexist;
			++pos;
			auto end = pos;
			auto begin = std::stoull(range.c_str() + pos, &end, 10);
			if (end == pos)
				begin = UINT64_MAX;
			pos = range.find('-');
			if (pos == range.npos)
				return noexist;
			++pos;
			if (pos == range.size())
				return {begin, UINT64_MAX };

			return{ begin, std::stoull(range.c_str() + pos, 0, 10) };
		}
		xhttper::query &get_query()
		{
			return query_;
		}
	private:
		friend class xserver;
		friend class uploader;
		friend class downloader;
		void reset()
		{
			body_.clear();
			boundary_.clear();
			is_close_ = false;
			in_callback_ = false;
			content_length_ = (std::size_t)-1;
			keepalive_ = -1;
			method_ = NUll;
			resp_.reset();
			parser_.reset();
		}
		std::string gen_session_id()
		{
			return std::to_string(
				std::chrono::high_resolution_clock::now().
				time_since_epoch().
				count());
		}
		bool check_upgrade()
		{
			using cmper = xutil::functional::strncasecmper;
			auto upgrade = parser_.get_header<cmper>("Upgrade");
			return cmper()(upgrade.c_str(), "websocket", strlen("websocket"));
		}
		void recv_callback(char *data, std::size_t len)
		{
			parser_.append(data, len);
			if (parser_.parse_req())
			{
				auto url = parser_.url();
				auto pos = url.find('?');
				if (pos != url.npos)
				{
					++pos;
					query_ = xhttper::query(url.c_str() + pos);
				}
				if (check_upgrade())
				{
					if (!handle_upgrade_)
					{
						resp_.set_status(404);
						resp_.done("Not Support Upgrade");
						close();
						return;
					}
					handle_upgrade_();
				}
				else
				{
					handle_request_();
					return;
				}
			}
			conn_.async_recv_some();
		}
		void close()
		{
			is_close_ = true;
			conn_.close();
			if (in_callback_ == false)
				close_callback_();
		}
		void do_receive()
		{
			resp_.send_buffer_ = [this](std::string &&buffer) 
			{
				send_buffers_.emplace_back(std::move(buffer));
				try_send();
			};
			conn_.regist_send_callback([this](std::size_t len) 
			{
				if (len == 0)
				{
					close();
					return;
				}
				is_send_ = false;
				try_send();
			});
			conn_.regist_recv_callback([this](char *data, std::size_t len)
			{
				if (len == 0)
				{
					close();
					return;
				}
				recv_callback(data, len);
			});
			conn_.async_recv_some();
		}
		void try_send()
		{
			if (is_send_ ||
			    is_close_ ||
			    send_buffers_.empty()) 
				return;

			is_send_ = true;
			conn_.async_send(std::move(send_buffers_.front()));
			send_buffers_.pop_front();
		}
		std::string path_;
		xhttper::query query_;
		std::string boundary_;
		bool is_close_ = false;
		bool is_send_ = false;
		int64_t id_ = 0;
		bool in_callback_ = false;
		std::function<void()> close_callback_;
		std::function<void()> handle_request_;
		std::function<void()> handle_upgrade_;
		std::function<void(std::string &&)> body_callback_;
		std::string body_;
		xnet::connection conn_;
		std::size_t content_length_ = (std::size_t)-1;
		int keepalive_ = -1;
		method method_ = NUll;
		xhttper::http_parser parser_;
		std::list<std::string> send_buffers_;
		response resp_;
		xserver *xserver_;
	};
}
