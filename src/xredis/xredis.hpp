#pragma once
#include <functional>
#include <memory>
#include "detail/detail.hpp"
namespace xredis
{
	using namespace detail;

	class redis
	{
	public:
		redis(xnet::proactor &_proactor, int max_slots = 16383)
			:proactor_(_proactor),
			max_slots_(max_slots)
		{
			client_.reset(new detail::client(proactor_.get_connector()));
		}
		template<typename CB>
		void req(const std::string &key, std::string &&buf, CB &&cb)
		{
			auto client = get_client(key);
			if (!client)
				throw std::runtime_error("not find client");

			client->req(std::move(buf), std::move(cb));
		}
		redis &set_addr(std::string ip, int port, bool cluster = true)
		{
			is_cluster_ = cluster;
			ip_ = ip;
			port_ = port;
			client_->regist_close_callback(
				std::bind(&redis::client_close_callback, 
					this, std::placeholders::_1));
			client_->regist_slots_moved_callback(
				std::bind(&redis::slots_moved_callback,
					this, std::placeholders::_1, std::placeholders::_2));
			client_->connect(ip_, port);
			if(is_cluster_)
				req_master_info();
			return *this;
		}
		redis &regist_connect_success_callback(std::function<void()> &&callback)
		{
			client_->regist_connect_success_callback(std::move(callback));
			return *this;
		}
		redis &regist_connect_failed_callback(std::function<void(const std::string &)> &&callback)
		{
			client_->regist_connect_failed_callback(std::move(callback));
			return *this;
		}
		redis &regist_cluster_init_callback(cluster_init_callback callback_)
		{
			cluster_init_callback_ = callback_;
			return *this;
		}
	private:
		void slots_moved_callback(const std::string & error, detail::client &)
		{
		}
		void client_close_callback(client &client)
		{
			auto itr = clients_.find(client.get_master_info().max_slot_);
			if(itr != clients_.end())
				clients_.erase(itr);
			if (client.get_master_info() == client_->get_master_info())
			{
				client_.release();
			}
		}
		void req_master_info()
		{
			client_->req(cmd_builder()({ "cluster","slots"}),
				[this](std::string &&error_code, cluster_slots &&slots) mutable {
				if(error_code.size())
				{
					return;
				}
				cluster_slots_.move_reset(std::move(slots));
			});
			client_->req(cmd_builder()({ "cluster","nodes" }),
				[this](std::string &&error_code, std::string && result) {
				if (error_code.size())
				{
					cluster_init_done(false);
					return;
				}
				cluster_nodes_callback(result);
				cluster_init_done(true);
			});
		}
		void cluster_nodes_callback(const std::string &result)
		{
			auto infos = std::move(master_info_parser()(result));
			if (infos != master_infos_)
			{
				update_client(infos);
				master_infos_ = std::move(infos);
			}
			
		}
		void cluster_init_done(bool result)
		{
			if (cluster_init_callback_)
				cluster_init_callback_({}, std::move(result));
			cluster_init_callback_ = nullptr;
		}
		detail::client *get_client(const std::string &key)
		{
			if(!is_cluster_ || key.empty())
				return client_.get();
			int slot = get_slot(key);
			auto itr = clients_.lower_bound(slot);
			if (itr == clients_.end() || 
				itr->second->get_master_info().min_slot_ > slot)
				return nullptr;
			return itr->second.get();
		}

		void update_client(const std::vector<master_info> & info)
		{
			for (auto &itr: info)
			{
				bool found = false;
				for(auto &ctr: clients_)
				{
					if (ctr.second->get_master_info().id_ == itr.id_)
					{
						found = true;
						if (ctr.second->get_master_info().min_slot_ != itr.min_slot_ )
						{
							ctr.second->get_master_info().min_slot_ = itr.min_slot_;
						}
						if (ctr.second->get_master_info().max_slot_ != itr.max_slot_ )
						{
							ctr.second->get_master_info().max_slot_ = itr.max_slot_;
							//fix clients_ key. 
							auto client = std::move(ctr.second);
							clients_.erase(clients_.find(ctr.first));
							clients_.emplace(client->get_master_info().max_slot_, std::move(client));
							break;
						}
					}
				}
				if (!found)
				{
					std::unique_ptr<detail::client> client(
						new detail::client(proactor_.get_connector()));
					client->get_master_info() = itr;
					client->connect(itr.ip_, itr.port_);
					clients_.emplace(itr.max_slot_, std::move(client));
				}
			}
		}
		master_info get_master_info(uint16_t slot)
		{
			for (auto &itr : master_infos_)
				if(itr.min_slot_ <= slot && slot < itr.max_slot_)
					return itr;
		}
		uint16_t get_slot(const std::string &key)
		{
			return crc16er()(key.c_str(), key.size()) % max_slots_;
		}
		std::string ip_;
		int port_;
		xnet::proactor &proactor_;
		std::unique_ptr<detail::client> client_;
		//cluster
		bool is_cluster_ = false;
		int max_slots_;
		cluster_init_callback cluster_init_callback_;
		std::map<int, std::unique_ptr<detail::client>> clients_;
		cluster_slots cluster_slots_;
		std::vector<master_info> master_infos_;
	};
}
