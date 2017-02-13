#pragma once
#include "detail/detail.hpp"
namespace xhttp_server
{
	class xserver
	{
	public:
		using request_handler = std::function<void(request &req, response &rsp)>;
		using on_message = std::function<void(request &req, response &rsp)>;

		xserver(std::size_t thread_count = std::thread::hardware_concurrency())
			:proactor_pool_(thread_count)
		{
		}
		xserver &bind(const std::string &ip, int port)
		{
			proactor_pool_.bind(ip, port);
			proactor_pool_.regist_accept_callback(
				[this](xnet::connection &&conn) {
				accept_callback(std::move(conn));
			});
			return *this;
		}
		void start()
		{
			proactor_pool_.regist_run_before([this] {
				for (auto &itr : before_runs_)
					itr();
			});
			before_runs_.emplace_back([this] {
				init_redis_session();
			});
			proactor_pool_.start();
		}
		void stop()
		{
			proactor_pool_.stop();
		}
		xserver &regist(request_handler handle)
		{
			request_handler_ = handle;
			return *this;
		}
		xserver &set_redis_addr(const std::string &ip, int port,bool cluster = false)
		{
			redis_ip_ = ip;
			redis_port_ = port;
			redis_cluster_ = cluster;
			return *this;
		}
		xnet::proactor_pool &get_proactor_pool()
		{
			return proactor_pool_;
		}
		xserver &regist_run_before(const std::function<void()> &callback)
		{
			before_runs_.push_back(callback);
			return *this;
		}
	private:
		void init_redis_session()
		{
			if (redis_ip_.size())
			{
				detail::init_redis_session(
					proactor_pool_,
					redis_ip_,
					redis_port_,
					redis_cluster_);
			}
		}
		void accept_callback(xnet::connection &&conn)
		{
			auto id = gen_id();
			auto req = std::make_shared<request>();
			req->id_ = id;
			req->conn_ = std::move(conn);
			req->xserver_ = this;
			req->handle_request_ = std::bind(&xserver::handle_request, this, std::ref(*req));
			req->close_callback_ = [&,id] { remove_request(id); };
			add_request(req->id_, req);
			req->do_receive();
		}
		void handle_request(request &req)
		{
			auto do_req = [&] {
				req.in_callback_ = true;
				request_handler_(req, req.resp_);
				if (req.is_close_)
				{
					remove_request(req.id_);
					return;
				}
				req.reset();
				req.do_receive();
			};
			xcoroutine::create(std::move(do_req));
		}
		void add_request(int64_t id, std::shared_ptr<request> & req)
		{
			std::lock_guard<std::mutex> lock(requests_mutex_);
			requests_.emplace(id, req);
		}
		void remove_request(int64_t id)
		{
			std::lock_guard<std::mutex> lock(requests_mutex_);
			requests_.erase(requests_.find(id));
		}
		int64_t gen_id()
		{
			return ++id_;
		}
		int64_t id_ = 0;
		std::mutex requests_mutex_;
		std::map<int64_t, std::shared_ptr<request>> requests_;
		request_handler request_handler_;
		xnet::proactor_pool proactor_pool_;
		std::string redis_ip_;
		int redis_port_ = 6379;
		bool redis_cluster_ = false;
		std::vector<std::function<void()>> before_runs_;
	};
}
