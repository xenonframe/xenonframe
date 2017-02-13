#pragma once
namespace xsocket_io
{
	class xserver
	{
	public:
		using handle_request_t = std::function<void(request&)> ;
		struct namespaces
		{
			namespaces(xserver &_xserver, const std::string &nsp)
				:xserver_(_xserver)
				, nsp_(nsp)
			{
			}
			template<typename T>
			xserver& on_connection(const T &handle)
			{
				xserver_.on_connection_handles_[nsp_] = handle;
				return xserver_;
			}
			namespaces& in(const std::string &room)
			{
				rooms_.insert(room);
				return *this;
			}
			namespaces& to(const std::string &room)
			{
				rooms_.insert(room);
				return *this;
			}
			template<typename T>
			void emit(const std::string &event, const T &msg)
			{
				return xserver_.do_emit(nsp_, rooms_, event, msg);
			}
		private:
			std::set<std::string> rooms_;
			std::string nsp_;
			xserver &xserver_;
		};
		xserver()
			:proactor_pool_(1)
		{
			init();
		}
		void on_connection(const std::function<void(socket&)> &handle)
		{
			on_connection_handles_["/"] = handle;
		}
		void on_request(const handle_request_t &handle)
		{
			handle_request_ = handle;
		}
		void bind(const std::string &ip, int port)
		{
			proactor_pool_.bind(ip, port);
		}
		void start()
		{
			proactor_pool_.start();
		}
		void set_static(const std::string &path)
		{
			static_path_ = path;
		}
		namespaces of(const std::string &nsp)
		{
			return namespaces(*this, nsp);
		}
		template<typename T>
		void emit(const std::string &event, const T &msg)
		{
			return do_emit("/", in_rooms_, event, msg);
		}
	private:
		template<typename T>
		void do_emit(const std::string &nsp, 
			const std::set<std::string> &rooms, 
			const std::string &event, 
			const T &msg)
		{
			xjson::obj_t obj;
			obj.add(event);
			obj.add(msg);

			detail::packet _packet;
			_packet.playload_ = obj.str();
			_packet.packet_type_ = detail::e_message;
			_packet.playload_type_ = detail::e_event;

			if (rooms.size())
			{
				for(auto &room: rooms)
				for (auto &itr : rooms_[nsp][room].socket_wptrs_)
				{
					auto sock = itr.second.lock();
					if (sock)
						sock->send_packet(_packet);
				}
				return;
			}
			for (auto &itr : sockets_)
			{
				if (nsp == "/" || itr.second->get_nsp() == nsp)
					itr.second->send_packet(_packet);
			}
		}
		int64_t uid()
		{
			static std::atomic_int64_t uid_ = 0;
			return uid_++;
		}
		void init()
		{
			proactor_pool_
				.regist_accept_callback([this](xnet::connection &&conn) {
				std::shared_ptr<request> 
					request_ptr(new request(std::move(conn)));
				auto id = uid();
				request_ptr->id_ = id;
				attach_request(request_ptr);
			});
		}
		void attach_request(std::shared_ptr<request> &request_ptr)
		{
			auto id = request_ptr->id_;
			std::weak_ptr<request> sess_wptr = request_ptr;

			request_ptr->on_request_ = [this, sess_wptr] {
				on_request(sess_wptr.lock());
			};

			request_ptr->close_callback_ = [id, this] {
				detach_request(id);
			};

			requests_.emplace(id, std::move(request_ptr));
		}

		std::shared_ptr<request> detach_request(int64_t id)
		{
			std::lock_guard<std::mutex> _lock_guard(requests_mutex_);

			std::shared_ptr<request> req_ptr;
			auto itr = requests_.find(id);
			assert(itr != requests_.end());
			req_ptr = std::move(itr->second);
			requests_.erase(itr);
			return req_ptr;
		}

		void on_request(const std::shared_ptr<request> &req)
		{
			auto &query = req->get_query();
			auto sid = query.get("sid");
			auto transport = query.get("transport");
			auto path = req->path();
			auto origin = req->get_entry("Origin");

			if (req->method() == "POST")
			{
				if (sid.size())
				{
					auto _socket = find_socket(sid);
					if (!_socket)
					{
						auto msg = detail::to_json(
							detail::error_msg::e_session_id_unknown);
						req->write(build_resp(msg, 400, origin));
						return;
					}
					_socket->on_packet(detail::decode_packet(req->body(), false));
					req->write(build_resp("ok", 200, origin, false));
				}
			}
			else if (req->method() == "GET")
			{
				auto url = req->url();
				if (path == "/socket.io/" && transport == "polling")
				{
					if (sid.empty())
					{
						return new_socket(req);
					}
					auto _socket = find_socket(sid);
					if (!_socket)
					{
						auto msg = detail::to_json(
							detail::error_msg::e_session_id_unknown);
						return req->write(build_resp(msg, 400, origin));
					}
					return _socket->on_polling(req);
				}
				if (url.find('?') == url.npos)
				{
					std::string filepath;
					if (check_static(url, filepath))
					{
						req->send_file(filepath);
						return;
					}
				}
				if(handle_request_)
					return handle_request_(*req);
				using ccmp = xutil::functional::strcasecmper;
				auto connection = req->get_entry("Connection");
				connection = xutil::functional::tolowerstr(connection);
				if (connection != "keep-alive")
				{
					if (connection.find("upgrade") != connection.npos)
					{
						auto _socket = find_socket(sid);
						if (!_socket)
						{
							auto msg = detail::to_json(
								detail::error_msg::e_session_id_unknown);
							return req->write(build_resp(msg, 400, origin));
						}
						return _socket->handle_Upgrade(req);
					}
				}
				
				return req->write(build_resp("", 404, origin, false));
			}
		}
		void new_socket(const std::shared_ptr<request> &req)
		{
			using namespace detail;
			auto sid = gen_sid();

			xjson::obj_t obj;
			packet _packet;
			open_msg msg;
			msg.pingInterval = ping_interval_;
			msg.pingTimeout = ping_timeout_;
			msg.upgrades = get_upgrades(req->query_.get("transport"));
			msg.sid = sid;

			obj = msg;
			_packet.binary_ = !!!req->get_entry("b64").size();
			_packet.packet_type_ = packet_type::e_open;
			_packet.playload_type_ = playload_type::e_null1;
			_packet.playload_ = obj.str();

			req->write(build_resp(
				encode_packet(_packet), 
				200, 
				req->get_entry("Origin"), 
				_packet.binary_));
			
			new_socket(sid);
		}
		
		std::string gen_sid()
		{
			static std::atomic_int64_t uid = 0;
			auto now = std::chrono::high_resolution_clock::now().
				time_since_epoch().count();
			auto id = uid.fetch_add(1) + now;
			auto sid = xutil::base64::encode(std::to_string(id));
			while(sid.size() && sid.back() =='=')
				sid.pop_back();
			return sid;
		}

		void new_socket(const std::string &sid)
		{
			std::shared_ptr<socket> sess(new socket());

			sess->close_callback_ = [this](auto &&...args) { 
				return socket_on_close(std::forward<decltype(args)>(args)...); 
			};
			sess->on_request_ = [this](auto &&...args) { 
				return on_request(std::forward<decltype(args)>(args)...); 
			};
			sess->on_connection_ = [this](auto &&...args) {
				return on_connection_callback(std::forward<decltype(args)>(args)...);
			};
			sess->join_room_ = [this](auto &&...args) {
				return join_room(std::forward<decltype(args)>(args)...);
			};
			sess->leave_room_ = [this](auto &&...args) {
				return left_room(std::forward<decltype(args)>(args)...);
			};
			sess->get_socket_from_room_ = [this](const std::string &nsp,
				const std::string &name)->auto &{
				return rooms_[nsp][name].socket_wptrs_;
			};
			sess->get_sockets_= [this]() ->auto &{
				return sockets_;
			};
			sess->set_timer_ = [this](int32_t timeout, std::function<bool()> &&handle)->
				std::size_t {
				return proactor_pool_.set_timer(timeout, std::move(handle));
			};
			sess->cancel_timer_ = [this](std::size_t timerid) {
				return proactor_pool_.cancel_timer(timerid);
			};

			
			sess->sid_ = sid;
			sess->timeout_ = ping_timeout_;
			sess->init();

			auto ptr = sess.get();
			do 
			{
				std::unique_lock<std::mutex> lock_g(session_mutex_);
				sockets_.emplace(sid, std::move(sess));
			} while (0);
			
		}
		bool check_static(const std::string& filename, std::string &filepath)
		{
			filepath = xutil::vfs::getcwd()() + static_path_ + filename;
			if (filename == "/")
				filepath = xutil::vfs::getcwd()() + static_path_ +  "index.html";
			if (xutil::vfs::file_exists()(filepath))
				return true;
			filepath.clear();
			return false;
		}

		bool check_sid(const std::string &sid)
		{
			std::unique_lock<std::mutex> lock_g(session_mutex_);
			return sockets_.find(sid) != sockets_.end();
		}
		std::shared_ptr<socket> find_socket(const std::string &sid)
		{
			auto itr = sockets_.find(sid);
			if (itr == sockets_.end())
				return nullptr;
			return itr->second;
		}

		void socket_on_close(const std::string &sid)
		{
			std::shared_ptr<socket> ptr;
			auto itr = sockets_.find(sid);
			if (itr != sockets_.end())
			{
				ptr = std::move(itr->second);
				sockets_.erase(itr);
			}
		}
		std::vector<std::string> get_upgrades(const std::string &transport)
		{
			return { "websocket" };
		}
		bool on_connection_callback(const std::string &nsp, socket &sock)
		{
			auto itr = on_connection_handles_.find(nsp);
			if (itr != on_connection_handles_.end())
				itr->second(sock);
			else 
				return false;
			return true;
		}
		void join_room(const std::string &nsp, 
			const std::string &name, std::shared_ptr<socket> &sock)
		{
			rooms_[nsp][name].socket_wptrs_[sock->get_sid()] = sock;
		}
		void left_room(const std::string &nsp, 
			const std::string &name, const std::string &sid)
		{
			auto room = rooms_[name][nsp].socket_wptrs_.erase(sid);
		}
		std::mutex session_mutex_;
		std::map<std::string, std::shared_ptr<socket>> sockets_;

		typedef std::weak_ptr<socket> weak_socket_ptr;
		struct weak_socket_ptrs
		{
			std::map<std::string, weak_socket_ptr> socket_wptrs_;
		};
		
		typedef std::map<std::string, weak_socket_ptrs> room;
		typedef std::map<std::string, room> rooms;
		rooms rooms_;

		std::mutex requests_mutex_;
		std::map<int64_t, std::shared_ptr<request>> requests_;

		std::map<std::string, std::function<void(socket&)>> on_connection_handles_;
		xnet::proactor_pool proactor_pool_;

		handle_request_t handle_request_;
		std::string static_path_;

		uint32_t ping_interval_ = 5000;
		uint32_t ping_timeout_ = 12000;

		std::set<std::string> in_rooms_;
	};
}