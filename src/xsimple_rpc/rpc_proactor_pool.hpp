#pragma once
#include <atomic>
namespace xsimple_rpc
{
	using namespace detail;
	class rpc_proactor_pool
	{
	public:
		rpc_proactor_pool(std::size_t io_thread_size_ = std::thread::hardware_concurrency())
			:proactor_pool_(io_thread_size_)
		{
			
		}
		~rpc_proactor_pool()
		{
			proactor_pool_.stop();
		}
		client connect(const std::string &ip, int port, int64_t timeout_millis = 0)
		{
			auto session = std::make_shared<rpc_session>();
			std::weak_ptr<rpc_session> session_wptr = session;
			auto msgbox_index = msgboxs_index_.fetch_add(1) % proactor_pool_.get_size();
			auto item = [ip, port, session_wptr, this, msgbox_index]
			{
				std::lock_guard<std::mutex> connections_locker(connectors_mutex_);
				auto connector = proactor_pool_.get_current_proactor().get_connector();
				connector_index_++;
				auto index = connector_index_;
				connector.bind_fail_callback([this, index, session_wptr](std::string errorc_code)
				{
					xutil::guard guard([this, index] { del_connector(index); });
					auto ptr = session_wptr.lock();
					if (!ptr)
						return;
					std::unique_lock<std::mutex> session_locker(ptr->mtx_);
					ptr->last_error_code_ = errorc_code;
					ptr->cv_.notify_one();
				}); 
				connector.bind_success_callback(
					[this, index, session_wptr, msgbox_index](xnet::connection &&conn)
				{
					xutil::guard guard([this, index] { del_connector(index); });
					auto ptr = session_wptr.lock();
					if (!ptr)
						return;
					std::unique_lock<std::mutex> session_locker(ptr->mtx_);
					ptr->conn_ = std::move(conn);
					ptr->init_conn();
					ptr->msgbox_index_ = msgbox_index;
					ptr->cv_.notify_one();
				});
				if (!connector.async_connect(ip, port))
				{
					auto ptr = session_wptr.lock();
					if (!ptr)
						return;
					std::unique_lock<std::mutex> session_locker(ptr->mtx_);
					ptr->last_error_code_ = connector.get_last_error();
					ptr->cv_.notify_one();
					return;
				}
				connectors_.emplace(index, std::move(connector));
			};
			proactor_pool_.post(std::move(item), msgbox_index);
			std::unique_lock<std::mutex> session_locker(session->mtx_);
			if (timeout_millis > 0)
			{
				auto res = session->cv_.wait_for(session_locker,
					std::chrono::milliseconds(timeout_millis), [&session] {
					return session->last_error_code_.size() || session->conn_.valid();
				});
				if (!res)
					throw std::runtime_error("connect_timeout");
			}
			else
			{
				session->cv_.wait(session_locker,[&session] {
					return session->last_error_code_.size() || session->conn_.valid();
				});
			}

			if (session->last_error_code_.size())
				throw std::runtime_error("connect error: " + session->last_error_code_);
			auto session_id = add_session(session);
			session->close_callback_ = [this, session_id] { del_session(session_id); };
			client _client;
			_client.send_req = [this, session_id](std::string &&buffer, int64_t req_id) {
				return do_rpc_call(session_id, std::move(buffer), req_id);
			};
			_client.close_rpc_ = [session_id, this, msgbox_index] {
				proactor_pool_.post([this, session_id] {
					del_session(session_id);
				}, msgbox_index);
			};
			return std::move(_client);
		}
		xnet::proactor_pool &get_proactor_pool()
		{
			return proactor_pool_;
		}
		void stop()
		{
			proactor_pool_.stop();
		}
		void start()
		{
			proactor_pool_.start();
		}
	private:
		using msgbox_t = xnet::msgbox<std::function<void()>>;
		using get_response = std::function<std::string(int64_t)>;
		using cancel_get_response = std::function<void()>;

		std::pair<get_response, cancel_get_response>
			do_rpc_call(int64_t session_id, std::string &&buffer, int64_t req_id)
		{
			auto item = std::make_shared<rpc_req>();
			item->req_buffer_ = std::move(buffer);
			item->req_id_ = req_id;

			std::lock_guard<std::mutex> locker(session_mutex_);
			auto itr = rpc_sessions_.find(session_id);
			if (itr == rpc_sessions_.end())
				throw std::runtime_error("can't find rpc_session");
			auto msg = item;
			auto session = itr->second;
			auto index = session->msgbox_index_;
			session->push_rpc_req(item);
			proactor_pool_.post([session] { session->do_send_rpc();}, index);

			return{ [session, item](int64_t timeout) {

				std::unique_lock<std::mutex> session_locker(session->mtx_);

				auto res = session->cv_.wait_for(session_locker, std::chrono::milliseconds(timeout), [item]{
					return item->status_ != rpc_req::status::e_null;
				});
				if (!res)
					throw rpc_timeout();
				else if (item->status_ == rpc_req::status::e_cancel)
					throw rpc_cancel();
				else if (item->status_ == rpc_req::status::e_rpc_error)
					throw rpc_error(item->result_);
				return item->result_;

			},[session, item] {
				item->status_ = rpc_req::status::e_cancel;
				session->cv_.notify_one();
			} };
		}

		void del_session(int64_t session_id)
		{
			std::lock_guard<std::mutex> locker(session_mutex_);
			auto itr = rpc_sessions_.find(session_id);
			if(itr != rpc_sessions_.end())
				rpc_sessions_.erase(itr);
		}
		int64_t add_session(std::shared_ptr<rpc_session> &session)
		{
			std::lock_guard<std::mutex> locker(session_mutex_);
			++session_id_;
			rpc_sessions_.emplace(session_id_, session);
			return session_id_;
		}

		int64_t add_connector(xnet::connector &&connector)
		{
			std::lock_guard<std::mutex> locker(connectors_mutex_);
			++connector_index_;
			connectors_.emplace(connector_index_, std::move(connector));
			return connector_index_;
		}
		void del_connector(int64_t index)
		{
			std::lock_guard<std::mutex> locker(connectors_mutex_);
			connectors_.erase(connectors_.find(index));
		}
		std::mutex session_mutex_;
		int64_t session_id_ = 1;
		std::map<int64_t, std::shared_ptr<rpc_session>> rpc_sessions_;

		std::atomic<uint64_t> msgboxs_index_ { 0 };

		std::mutex connectors_mutex_;
		int64_t connector_index_ = 0;
		std::map<int64_t, xnet::connector> connectors_;

		xnet::proactor_pool proactor_pool_;
	};
}
