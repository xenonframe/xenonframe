#pragma once
namespace xsocket_io
{
	class socket: public std::enable_shared_from_this<socket>
	{
	public:
		struct broadcast_t
		{
			template<typename T>
			void emit(const std::string &event_name, const T &msg)
			{
				socket_->do_broadcast(rooms_, event_name, msg);
				rooms_.clear();
			}
			void to(const std::string &name)
			{
				rooms_.insert(name);
			}
			void in(const std::string &name)
			{
				rooms_.insert(name);
			}
		private:
			friend class socket;
			std::set<std::string> rooms_;
			socket *socket_;
		};
		socket()
		{
		}
		~socket()
		{
			if (timer_id_)
				cancel_timer_(timer_id_);

			auto rooms = rooms_;
			for (auto &itr : rooms)
				leave(itr);
		}
		template<typename Lambda>
		void on(const std::string &event_name, const Lambda &lambda)
		{
			regist_event(event_name, xutil::to_function(lambda));
		}

		template<typename T>
		void emit(const std::string &event_name,const T &msg)
		{
			xjson::obj_t obj;
			obj.add(event_name);
			obj.add(msg);

			detail::packet _packet;
			_packet.packet_type_ = detail::packet_type::e_message;
			_packet.playload_type_ = detail::playload_type::e_event;
			_packet.playload_ = obj.str();
			
			if(in_rooms_.empty())
				return send_packet(_packet);
			broadcast_room(in_rooms_, _packet);
			in_rooms_.clear();
		}
		
		std::string get_sid()
		{
			return sid_;
		}
		void set(const std::string &key, const std::string &value)
		{
			properties_[key] = value;
		}
		std::string get(const std::string &key)
		{
			return properties_[key];
		}
		std::string get_nsp()
		{
			return nsp_;
		}
		void join(const std::string &room)
		{
			if (rooms_.find(room) == rooms_.end())
			{
				rooms_.insert(room);
				join_room_(nsp_, room, shared_from_this());
				return;
			}
		}
		void leave(const std::string &room)
		{
			rooms_.erase(room);
			leave_room_(nsp_, room, sid_);
		}
		socket &in(const std::string &room)
		{
			in_rooms_.insert(room);
			return *this;
		}
		socket &to(const std::string &room)
		{
			in_rooms_.insert(room);
			return *this;
		}
		broadcast_t broadcast;
	private:
		socket(socket &&sess) = delete;
		socket(socket &sess) = delete;
		socket &operator = (socket &&sess) = delete;
		socket &operator = (socket &sess) = delete;

		friend class xserver;

		struct cookie_opt
		{
			bool httpOnly_ = true;
			bool secure_ = false;
			std::string path_ = "/";
			int64_t expires_ = 0;

			enum class sameSite
			{
				Null,
				Strict,
				Lax
			};
			sameSite sameSite_ = sameSite::Null;
		};
		void broadcast_room(std::set<std::string> &rooms, 
			detail::packet &_packet)
		{
			std::set<std::string> ids;
			for (auto &itr : rooms)
			{
				auto &sockets = get_socket_from_room_(nsp_, itr);
				for (auto &sock : sockets)
				{
					auto ptr = sock.second.lock();
					if (!ptr)
						continue;
					if (ptr->nsp_ != nsp_)
						continue;
					if (ids.find(sock.first) == ids.end())
					{
						ptr->send_packet(_packet);
						ids.insert(sock.first);
					}
				}
			}
		}
		template<typename T>
		void do_broadcast(std::set<std::string> &rooms_, 
			const std::string &event_name, const T &msg)
		{
			xjson::obj_t obj;
			obj.add(event_name);
			obj.add(msg);

			detail::packet _packet;
			_packet.packet_type_ = detail::packet_type::e_message;
			_packet.playload_type_ = detail::playload_type::e_event;
			_packet.playload_ = obj.str();

			if (rooms_.empty())
			{
				auto &sockets = get_sockets_();
				for (auto &itr : sockets)
				{
					if (itr.first == sid_ || itr.second->nsp_ != nsp_)
						continue;
					itr.second->send_packet(_packet);
				}
				return;
			}
			broadcast_room(rooms_, _packet);
		}
		void send_packet(detail::packet &_packet)
		{
			_packet.nsp_ = nsp_;
			_packet.binary_ = !b64_;
			if (!upgrade_)
			{
				auto _request = polling_.lock();
				if (_request)
				{
					auto msg = detail::encode_packet(_packet);
					_request->write(build_resp(msg, 200, origin_, !b64_));
					return;
				}
				return packets_.emplace_back(_packet);
			}
			if(packets_.size() || is_send_)
				return packets_.emplace_back(_packet);
			is_send_ = true;
			xwebsocket::frame_maker _frame_maker;
			auto msg = detail::encode_packet(_packet, true);
			auto frame = _frame_maker.
				set_fin(true).
				set_frame_type(xwebsocket::frame_type::e_text).
				make_frame(msg.c_str(), msg.size());
			ws_conn_.async_send(std::move(frame));
		}
		void flush()
		{
			if (packets_.empty())
				return;

			std::string buffer;
			for (auto &itr : packets_)
			{
				itr.binary_ = !b64_;
				buffer += detail::encode_packet(itr, upgrade_);
			}
			packets_.clear();
			if (!upgrade_)
			{
				auto _request = polling_.lock();
				if (_request)
					return _request->write(build_resp(buffer, 200, origin_, !b64_));
				assert(false);
			}
			is_send_ = true;
			xwebsocket::frame_maker _frame_maker;
			auto frame = _frame_maker.
				set_fin(true).
				set_frame_type(xwebsocket::frame_type::e_text).
				make_frame(buffer.c_str(), buffer.size());
			ws_conn_.async_send(std::move(frame));
		}
		void handle_Upgrade(const std::shared_ptr<request> &req)
		{
			using ccmp = xutil::functional::strcasecmper;
			if (!ccmp()(req->get_entry("Upgrade").c_str(), "websocket"))
				return req->write(build_resp("", 404, req->get_entry("Origin"), false));

			upgrade_ = true;
			b64_ = !!req->get_query().get("b64").size();
			auto Sec_WebSocket_Key = req->get_entry("Sec-WebSocket-Key");
			auto Sec_WebSocket_Version = req->get_entry("Sec-WebSocket-Version");
			auto Sec_WebSocket_Protocol = req->get_entry("Sec-WebSocket-Protocol");

			frame_parser_.regist_frame_callback([this](std::string &&data, 
				xwebsocket::frame_type type, bool fin) {
				
				if (type == xwebsocket::frame_type::e_binary)
					return xcoroutine::create([&] {
					on_packet(detail::decode_packet(data, !b64_, true));
				});
				else if (type == xwebsocket::frame_type::e_text)
				{
					return xcoroutine::create([&] {
						on_packet(detail::decode_packet(data, !b64_, true));
					});
				}
				else if (type == xwebsocket::frame_type::e_ping)
					return pong();
				else if (type == xwebsocket::frame_type::e_connection_close)
					is_close_ = true;
			});
			ws_conn_ = std::move(req->detach_connection());
			ws_conn_.regist_send_callback([this](std::size_t len) {

				if (!len)
					return on_close();
				is_send_ = false;
				flush();
			});
			ws_conn_.regist_recv_callback([this](char *data, std::size_t len) {

				if(!len)
					return on_close();
				frame_parser_.do_parse(data, (uint32_t)len);
				if (is_close_)
					return on_close();
				ws_conn_.async_recv_some();
			});


			is_send_ = true;
			auto handshake = xwebsocket::make_handshake(Sec_WebSocket_Key, Sec_WebSocket_Protocol);
			ws_conn_.async_send(std::move(handshake));
			ws_conn_.async_recv_some();

			std::string remain = req->get_http_parser().get_string();
			if (remain.size())
				frame_parser_.do_parse(remain.c_str(), (uint32_t)remain.size());
			//send noop to last polling.
			if (polling_.lock())
				send_noop(polling_.lock());
			req->close();
		}
		std::string make_cookie(const std::string &key, 
			const std::string &value, 
			cookie_opt opt)
		{
			std::string buffer;
			buffer.append(key);
			buffer.append("=");
			buffer.append(value);
			if (opt.path_.size())
			{
				buffer.append("; path=");
				buffer.append(opt.path_);
			}
			if (opt.secure_)
			{
				buffer.append("; Secure");
			}
			if (opt.httpOnly_)
			{
				buffer.append("; HttpOnly");
			}
			if (opt.expires_ > 0)
			{
				buffer.append("; Expires=");
				buffer.append(std::to_string(opt.expires_));
			}
			buffer.pop_back();
			return buffer;
		}
		void pong(const std::string &playload = {})
		{
			detail::packet _packet;
			_packet.packet_type_ = detail::packet_type::e_pong;
			_packet.playload_ = playload;
			send_packet(_packet);
		}

		void connect_ack(const std::shared_ptr<request>&req)
		{
			connect_ack_ = true;
			detail::packet _packet;
			_packet.packet_type_ = detail::packet_type::e_message;
			_packet.playload_type_ = detail::playload_type::e_connect;
			_packet.binary_ = !b64_;

			packets_.push_back(_packet);
			if (!on_connection_(nsp_, *this))
			{
				_packet.playload_ = "\"Invalid namespace\"";
				_packet.playload_type_ = detail::e_error;
				on_connection_ = nullptr;
			}
			if (nsp_ != "/")
			{
				_packet.nsp_ = nsp_;
				packets_.push_back(_packet);
			}
			std::string buffer;
			for (auto &itr : packets_)
			{
				itr.binary_ = !b64_;
				buffer += detail::encode_packet(itr);
			}
			packets_.clear();
			return req->write(build_resp(buffer, 200, origin_, !b64_));
		}
		void on_packet(const std::list<detail::packet> &_packet)
		{
			for (auto &itr : _packet)
			{
				switch (itr.packet_type_)
				{
				case detail::packet_type::e_ping:
				{
					pong(itr.playload_);
					set_timeout();
					break;
				}
				case detail::packet_type::e_close:
				{
					is_close_ = true;
					break;
				}
				case detail::packet_type::e_message:
				{
					switch (itr.playload_type_)
					{
					case detail::playload_type::e_event:
					{
						if (nsp_ == itr.nsp_)
							on_message(itr);
					}
					case detail::playload_type::e_connect:
					{
						nsp_ = itr.nsp_;
					}
					default:
						break;
					}
				}
				default:
					break;
				}
			}
		}

		void on_message(const detail::packet &_packet)
		{
			try
			{
				auto obj = xjson::build(_packet.playload_);
				std::string event = obj.get<std::string>(0);
				auto itr = event_handles_.find(event);
				if (itr == event_handles_.end())
					return;
				if(obj.size() == 1)
					itr->second(xjson::obj_t());
				else
					itr->second(obj.get(1));
			}
			catch (const std::exception& e)
			{
				COUT_ERROR(e);
			}
		}
		void send_noop(const std::shared_ptr<request> &req)
		{
			detail::packet _packet;
			_packet.packet_type_ = detail::e_noop;
			_packet.binary_ = !b64_;
			auto msg = detail::encode_packet(_packet);
			return req->write(build_resp(msg, 200, origin_, !b64_));
		}
		void on_polling(const std::shared_ptr<request> &req)
		{
			auto method = req->method();
			auto path = req->path();
			auto url = req->url();
			origin_ = req->get_entry("Origin");
			b64_ = !!req->get_query().get("b64").size();
			
			if (!connect_ack_)
			{
				return connect_ack(req);
			}
			if (upgrade_)
				return send_noop(req);

			polling_ = req;

			flush();
		}

		void init()
		{
			broadcast.socket_ = this;
		}

		void regist_event(const std::string &event_name,
			std::function<void(xjson::obj_t&obj)> &&handle_)
		{
			std::function<void(xjson::obj_t&obj)> handle;
			auto func = [handle = std::move(handle_)](xjson::obj_t &obj){
				handle(obj);
			};
			if (event_handles_.find(event_name) != event_handles_.end())
				throw std::runtime_error("event");
			event_handles_.emplace(event_name, std::move(func));
		}

		void regist_event(const std::string &event_name, std::function<void()> &&handle_)
		{
			std::function<void(xjson::obj_t&obj)> handle;
			auto func = [handle = std::move(handle_)](xjson::obj_t &){
				handle();
			};
			if (event_handles_.find(event_name) != event_handles_.end())
				throw std::runtime_error("event");
			event_handles_.emplace(event_name, std::move(func));
		}

		void on_sid(const std::string &sid)
		{
			sid_ = sid;
		}
		void on_close()
		{
			if (connect_ack_)
			{
				auto itr = event_handles_.find("disconnect");
				if (itr != event_handles_.end())
					itr->second(xjson::obj_t());
			}
			close_callback_(sid_);
		}
		void set_timeout()
		{
			if (timer_id_)
				cancel_timer_(timer_id_);

			timer_id_ = set_timer_(timeout_, [this] {
				detail::packet _packet;
				_packet.packet_type_ = detail::e_close;
				send_packet(_packet);
				timer_id_ = 0;
				on_close();
				return false;
			});
		}
		bool connect_ack_ = false;
		bool upgrade_ = false;
		bool b64_ = false;
		bool is_close_ = false;
		int32_t timeout_;
		std::string origin_;

		std::list<detail::packet> packets_;
		std::map<std::string, std::function<void(xjson::obj_t&)>> event_handles_;
		std::function<void(const std::string &)> close_callback_;
		std::function<void(std::shared_ptr<request>)> on_request_;
		std::function<std::map<std::string, std::shared_ptr<socket>>&()> get_sockets_;
		
		std::function<std::size_t(int32_t, std::function<bool()> &&)> set_timer_;
		std::function<void(std::size_t)> cancel_timer_;
		std::size_t timer_id_ = 0;

		xnet::connection conn_;
		std::string sid_ ;
		std::string nsp_ = "/";

		std::weak_ptr<request> polling_;
		std::map<std::string, std::string > properties_;

		std::function<bool(const std::string &, socket&)> on_connection_;
		

		std::set<std::string> rooms_;
		std::set<std::string> in_rooms_;

		std::function<void(const std::string &, const std::string &, std::shared_ptr<socket>&)> join_room_;
		std::function<void(const std::string &, const std::string &, const std::string &)> leave_room_;

		using sockets_in_room = std::map<std::string, std::weak_ptr<socket>>;
		std::function<sockets_in_room& (const std::string &, const std::string &)> get_socket_from_room_;

		xwebsocket::frame_parser frame_parser_;
		xnet::connection ws_conn_;
		bool is_send_ = false;
	};
}