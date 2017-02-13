#pragma once
namespace xsimple_rpc
{
	class async_client
	{
	public:
		async_client(xnet::connection &&conn)
			:conn_(std::move(conn))
		{
			init();
		}
		~async_client()
		{
			conn_.close();
		}
		
		async_client(async_client &&client)
		{
			move_reset(std::move(client));
		}
		async_client &operator=(async_client && client)
		{
			move_reset(std::move(client));
			return *this;
		}
		template<typename Proto, typename Tuple, typename Callback>
		void rpc_call(Tuple &&tuple, Callback &&callback)
		{
			rpc_call_help(xutil::function_traits<typename Proto::func_type>(), 
				Proto::rpc_name(), 
				std::move(tuple), 
				xutil::to_function(std::forward<Callback>(callback)));
		}
		template<typename Proto, typename Callback>
		void rpc_call(Callback &&callback)
		{
			rpc_call_help(xutil::function_traits<typename Proto::func_type>(),
				Proto::rpc_name(),
				xutil::to_function(std::forward<Callback>(callback)));
		}
	private:
		enum step
		{
			e_msg_len,
			e_msg_data,
		};
		void rpc_call_help(xutil::function_traits<void()>,
			const std::string &rpc_name,
			std::function<void(const std::string&)>&&callback)
		{
			using detail::make_req;
			auto id = gen_id();
			auto req = make_req(rpc_name, id);
			callbacks_.emplace_back(id, [this, callback](const std::string &error_code, const std::string &) {
				callback(error_code);
			});
			if (is_send_)
			{
				send_buffers_.emplace_back(std::move(req));
				return;
			}
			is_send_ = true;
			conn_.async_send(std::move(req));
		}

		template<typename Ret>
		void rpc_call_help(xutil::function_traits<Ret()>,
			const std::string &rpc_name,
			std::function<void(const std::string &, typename std::remove_reference<Ret>::type)>&&callback)
		{
			using detail::make_req;
			auto id = gen_id();
			auto req = make_req(rpc_name, id);
			callbacks_.emplace_back(id, [this, callback](const std::string &error_code, const std::string &resp) {
				uint8_t *ptr = (uint8_t *)resp.data();
				uint8_t *end = (uint8_t *)resp.data() + resp.size();
				callback(error_code, endec::get<Ret>(ptr, end));
			});
			if (is_send_)
			{
				send_buffers_.emplace_back(std::move(req));
				return;
			}
			is_send_ = true;
			conn_.async_send(std::move(req));
		}

		template<typename Ret, typename ...Args>
		void rpc_call_help(xutil::function_traits<Ret(Args...)>,
			const std::string &rpc_name, 
			std::tuple<typename endec::remove_const_ref<Args>::type&&...> &&_tuple, 
			std::function<void(const std::string &, typename std::remove_reference<Ret>::type)>&&callback)
		{
			using detail::make_req;
			auto id = gen_id();
			auto req = make_req(rpc_name, id, std::move(_tuple));
			callbacks_.emplace_back(id, [this, callback](const std::string &error_code, std::string &resp) {
				uint8_t *ptr = (uint8_t *)resp.data();
				uint8_t *end = (uint8_t *)resp.data() + resp.size();
				callback(error_code, endec::get<Ret>(ptr, end));
			});
			if (is_send_)
			{
				send_buffers_.emplace_back(std::move(req));
				return;
			}
			is_send_ = true;
			conn_.async_send(std::move(req));
		}
		void close()
		{

		}
		int64_t gen_id()
		{
			static int64_t id = 0;
			return ++id;
		}
		void init()
		{
			conn_.regist_recv_callback([this](char *data, std::size_t len) {

				if (len == 0)
					goto _close;

				if (step_ == e_msg_len)
				{
					step_ = e_msg_data;
					uint8_t *ptr = (uint8_t*)data;
					uint8_t *end = (uint8_t*)data + len;
					uint32_t msg_len = endec::get<uint32_t>(ptr, end);
					if (msg_len < min_rpc_msg_len())
						goto _close;
					conn_.async_recv(msg_len - sizeof(uint32_t));
					return;
				}
				else if (step_ == e_msg_data)
				{
					step_ = e_msg_len;
					uint8_t *ptr = (uint8_t*)data;
					uint8_t *end = (uint8_t*)data + len;
					try
					{
						if (endec::get<std::string>(ptr, end) != magic_code)
							goto _close;
						auto req_id = endec::get<int64_t>(ptr, end);
						auto resp = endec::get<std::string>(ptr, end);
						if (callbacks_.empty() || callbacks_.front().first != req_id)
							goto _close;
						callbacks_.front().second({}, resp);
						callbacks_.pop_front();
						conn_.async_recv(sizeof(uint32_t));
						return;
					}
					catch (const std::exception &e)
					{
						std::cout << e.what() << std::endl;
					}
				}
			_close:
				close();
				return;
			});

			conn_.regist_send_callback([this](std::size_t len) 
			{
				if (len == 0)
				{
					conn_.close();
					return;
				}
				if (send_buffers_.empty())
				{
					is_send_ = false;
					return;
				}
				conn_.async_send(std::move(send_buffers_.front()));
				send_buffers_.pop_front();
			});
			conn_.async_recv(sizeof(uint32_t));
		}
		void move_reset(async_client &&client)
		{
			if (&client == this)
				return;
			step_ = client.step_;
			is_send_ = client.is_send_;
			send_buffers_ = std::move(send_buffers_);
			callbacks_ = std::move(callbacks_);
			conn_ = std::move(conn_);
		}
		using callback_t = std::function<void(const std::string &, std::string &)>;

		step step_ = e_msg_len;
		bool is_send_ = false;
		std::list<std::string> send_buffers_;
		std::list<std::pair<int64_t, callback_t>> callbacks_;
		xnet::connection conn_;
	};
}
