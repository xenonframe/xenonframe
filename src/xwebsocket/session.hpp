#pragma once
namespace xwebsocket
{
	class session
	{
	public:
		
		using frame_callback = frame_parser::frame_callback;

		session()
		{

		}

		session(session &&sess)
		{
			reset_move(std::move(sess));
		}

		session &operator=(session &&sess)
		{
			reset_move(std::move(sess));
			return *this;
		}
		session &regist_frame_callback(const frame_callback &callback)
		{
			frame_parser_.regist_frame_callback(callback);
			return *this;
		}
		session &regist_close_callback(const std::function<void()> &callback)
		{
			close_callback_ = callback;
			return *this;
		}
		void send_text(const std::string &msg)
		{
			send_data(frame_maker_.
				set_fin(true).
				set_frame_type(frame_type::e_text).
				make_frame(msg.c_str(), msg.size()));
		}
		void send_close()
		{
			send_data(frame_maker_.
				set_fin(true).
				set_frame_type(frame_type::e_connection_close).
				make_frame(nullptr, 0));
		}
		std::string req_path()
		{
			return path_;
		}
	private:
		void send_data(std::string &&data)
		{
			if (!is_send_)
				return do_send(std::move(data));
			send_buffer_list_.emplace_back(std::move(data));
		}
		friend class xserver;

		session(xnet::connection &&conn)
			:conn_(std::move(conn))
		{

		}

		void reset_move(session &&other)
		{
			if (&other == this)
				return;
			is_send_ = other.is_send_;
			id_ = other.id_;
			send_buffer_list_ = std::move(other.send_buffer_list_);
			close_callback_ = std::move(other.close_callback_);
			frame_parser_ = std::move(other.frame_parser_);
			conn_ = std::move(other.conn_);

			using namespace std::placeholders;
			conn_.
				regist_recv_callback(std::bind(&session::recv_callback, this, _1, _2)).
				regist_send_callback(std::bind(&session::send_callback, this, _1));
		}
		
		void do_send(std::string &&buffer)
		{
			assert(!is_send_);
			conn_.async_send(std::move(buffer));
			is_send_ = true;
		}
		void init()
		{
			using namespace std::placeholders;
			conn_.
				regist_recv_callback(std::bind(&session::recv_callback, this, _1, _2)).
				regist_send_callback(std::bind(&session::send_callback, this, _1)).
				async_recv_some();
		}
		void recv_callback(char *data, std::size_t len)
		{
			if (len == 0)
			{
				on_close();
				return;
			}

			if (!make_handshake_)
			{
				http_parser_.append(data, len);
				if (http_parser_.parse_req())
				{
					using cmper = xutil::functional::strncasecmper;
					make_handshake_ = true;
					auto upgrade = http_parser_.get_header<cmper>("Upgrade");
					auto Sec_WebSocket_Key = http_parser_.get_header<cmper>("Sec-WebSocket-Key");
					auto Sec_WebSocket_Protocol = http_parser_.get_header<cmper>("Sec-WebSocket-Protocol");
					if (!xutil::functional::strcasecmper()(upgrade.c_str(), "websocket")||
						Sec_WebSocket_Key.empty())
					{
						response_404();
						on_close();
					}
					send_data(make_handshake(Sec_WebSocket_Key, Sec_WebSocket_Protocol));
					path_ = http_parser_.url();

					auto buffer = http_parser_.get_string();

					if (buffer.size())
						return recv_callback((char*)buffer.c_str(), buffer.size());
				}
			}
			else 
			{
				frame_parser_.do_parse(data, (uint32_t)len);
			}
			conn_.async_recv_some();
		}
		void response_404()
		{
			http_buider_.set_status(404);
			send_data(http_buider_.build_resp());
		}
		void send_callback(std::size_t len)
		{
			is_send_ = false;
			if (len == 0)
			{
				on_close();
				return;
			}
			
			if (send_buffer_list_.empty())
				return;
			do_send(std::move(send_buffer_list_.front()));
			send_buffer_list_.pop_front();
		}
		
		void on_close()
		{
			conn_.close();
			if (close_callback_)
				close_callback_();
		}

		bool make_handshake_ = false;
		bool is_send_ = false;
		int64_t id_ = 0;
		std::list<std::string> send_buffer_list_;
		std::function<void()> close_callback_;

		frame_parser frame_parser_;
		frame_maker frame_maker_;
		xhttper::http_parser http_parser_;
		xhttper::http_builder http_buider_;
		xnet::connection conn_;

		std::string path_;
	};
}