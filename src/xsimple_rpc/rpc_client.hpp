#pragma once
namespace xsimple_rpc
{
	class client
	{
	private:
		using get_response = std::function<std::string(int64_t)>;
		using cancel_get_response = std::function<void()>;
		using send_rpc_result = std::pair <get_response, cancel_get_response>;
		friend class rpc_proactor_pool;
	public:
		
		client(client &&other)
		{
			move_reset(std::move(other));
		}
		client &operator=(client && other)
		{
			move_reset(std::move(other));
			return *this;
		}
		~client()
		{
			if (close_rpc_)
				close_rpc_();
		}
		client& set_timeout(int64_t rpc_timeout)
		{
			rpc_timeout_ = rpc_timeout;
			return *this;
		}
		template<typename Proto, typename ...Args>
		auto rpc_call(Args ...args)
		{
			return rpc_call_impl(xutil::function_traits<typename Proto::func_type>(),
				Proto::rpc_name(),
				std::forward<Args>(args)...);
		}
		template<typename Proto>
		auto rpc_call()
		{
			return rpc_call_impl(xutil::function_traits<typename Proto::func_type>(),
				Proto::rpc_name());
		}
	private:
		client()
		{

		}
		void move_reset(client &&other)
		{
			if (&other == this)
				return;
			std::lock_guard <std::mutex> locker(other.mutex_);
			send_req = std::move(other.send_req);
			close_rpc_ = std::move(other.close_rpc_);
		}

		template<typename Ret, typename ...Args>
		Ret rpc_call_impl(
			const xutil::function_traits<Ret(Args...)>&,
			const std::string &rpc_name,
			typename std::remove_reference<typename std::remove_const<Args>::type>::type&& ...args)
		{
			using detail::make_req;
			auto req_id = gen_req_id();
			auto buffer = make_req(rpc_name, req_id, std::forward_as_tuple(args...));
			if (!send_req)
				throw std::logic_error("client isn't connected");

			auto result = send_req(std::move(buffer), req_id);
			set_cancel_get_response(std::move(result.second));
			xutil::guard guard{[&] {
				reset_cancel_get_response();
			}};
			auto resp = result.first(rpc_timeout_);
			uint8_t *ptr = (uint8_t*)resp.data();
			uint8_t *end = ptr + resp.size();
			auto res = detail::endec::get<Ret>(ptr, end);
			if (ptr != end)
				throw std::runtime_error("rpc resp error");
			return std::move(res);
		}

		template<typename Ret>
		Ret rpc_call_impl(const xutil::function_traits<Ret(void)>&, const std::string &rpc_name)
		{
			using detail::make_req;
			auto req_id = gen_req_id();
			auto buffer = make_req(rpc_name, req_id);
			if (!send_req)
				throw std::logic_error("client isn't connected");

			auto result = send_req(std::move(buffer), req_id);
			set_cancel_get_response(std::move(result.second));
			xutil::guard guard{[&] {
				reset_cancel_get_response();
			}};
			auto resp = result.first(rpc_timeout_);
			uint8_t *ptr = (uint8_t*)resp.data();
			uint8_t *end = ptr + resp.size();
			auto res = detail::endec::get<Ret>(ptr, end);
			if (ptr != end)
				throw std::runtime_error("rpc resp error");
			return std::move(res);
		}

		template<typename ...Args>
		void rpc_call_impl(
			const xutil::function_traits<void(Args...)>&,
			const std::string &rpc_name,
			typename std::remove_reference<typename std::remove_const<Args>::type>::type&& ...args)
		{
			using detail::make_req;
			auto req_id = gen_req_id();
			auto buffer = make_req(rpc_name, req_id, std::forward_as_tuple(args...));
			if(!send_req)
				throw std::logic_error("client isn't connected");
			auto get_resp = send_req(std::move(buffer), req_id);
			set_cancel_get_response(std::move(get_resp.second));
			xutil::guard guard([&] {
				reset_cancel_get_response();
			});
			get_resp.first(rpc_timeout_);
		}

		void rpc_call_impl(const xutil::function_traits<void(void)>&,
			const std::string &rpc_name)
		{
			using detail::make_req;
			auto req_id = gen_req_id();
			auto buffer = make_req(rpc_name, req_id);
			if (!send_req)
				throw std::logic_error("client isn't connected");
			auto get_resp = send_req(std::move(buffer), req_id);
			set_cancel_get_response(std::move(get_resp.second));
			xutil::guard guard([&] {
				reset_cancel_get_response();
			});
			get_resp.first(rpc_timeout_);
		}

		int64_t gen_req_id()
		{
			return req_id_++;
		}
		void reset_cancel_get_response()
		{
			std::lock_guard<std::mutex> locker(mutex_);
			cancel_get_response_ = nullptr;
		}
		void set_cancel_get_response(cancel_get_response && handle)
		{
			std::lock_guard<std::mutex> locker(mutex_);
			cancel_get_response_ = std::move(handle);
		}
		int64_t req_id_ = 1;
		int64_t rpc_timeout_ = 30000;
		std::mutex mutex_;
		cancel_get_response cancel_get_response_;
		std::function<send_rpc_result(std::string &&, int64_t)> send_req;
		std::function<void()> close_rpc_;
	};
}
