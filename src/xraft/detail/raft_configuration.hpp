#pragma once
namespace xraft
{
	namespace detail
	{
		class raft_config_mgr
		{
		public:
			raft_config_mgr()
			{

			}
			int get_majority()
			{
				return (int)nodes_.size() / 2 + 1;
			}
			void set(raft_config::nodes nodes)
			{
				nodes_ = nodes;
			}
			raft_config::nodes &get_nodes()
			{
				return nodes_;
			}
		private:
			raft_config::nodes nodes_;
		};
	}
}