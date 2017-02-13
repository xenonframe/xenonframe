#pragma once
namespace xredis
{
	struct cluster_slots
	{
		cluster_slots()
		{

		}
		cluster_slots(cluster_slots && self)
		{
			move_reset(std::move(self));
		}
		cluster_slots &operator=(cluster_slots &self)
		{
			move_reset(std::move(self));
			return *this;
		}
		void move_reset(cluster_slots && self)
		{
			if(&self == this)
				return;
			cluster_ = std::move(self.cluster_);
		}
		struct cluster_info
		{
			struct node
			{
				int port_;
				std::string ip_;
				std::string id_;
			};
			int min_slots_;
			int max_slots_;
			std::vector<node> node_;
		};
		std::vector<cluster_info> cluster_;
	};
	struct master_info
	{
		bool operator==(const master_info & self)const 
		{
			return min_slot_ == self.min_slot_ &&
				max_slot_ == self.max_slot_ &&
				id_ == self.id_ &&
				ip_ == self.ip_ &&
				port_ == self.port_;
		}
		bool operator != (const master_info & self)const
		{
			return !(*this == self);
		}
		int min_slot_;
		int max_slot_;
		std::string id_;
		std::string ip_;
		int  port_;
	};

	struct pmessage
	{
		std::string source_topic;
		std::string dest_topic;
		std::string msg;
	};
}