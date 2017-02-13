#pragma once
namespace xraft
{
	namespace detail
	{
		struct vote_request
		{
			int64_t term_ = 0;
			std::string candidate_;
			int64_t last_log_index_ = 0;
			int64_t last_log_term_ = 0;

			XENDEC(term_, candidate_, last_log_index_, last_log_term_);
		};

		struct vote_response 
		{
			int64_t term_ = 0;
			bool vote_granted_ = false;
			bool log_ok_ = false;

			XENDEC(term_, vote_granted_, log_ok_);
		};

		struct log_entry
		{
			enum type
			{
				e_append_log,
				e_configuration
			};
			int64_t index_ = 0;
			int64_t term_ = 0;
			type type_ = type::e_append_log;
			std::string log_data_;

			XENDEC(index_, term_, type_, log_data_);

			std::size_t bytes() const
			{
				return
					endec::get_sizeof(index_) +
					endec::get_sizeof(term_) +
					endec::get_sizeof(std::underlying_type<type>::type()) +
					endec::get_sizeof(log_data_);
			}
			std::string to_string() const
			{
				std::string buffer;
				buffer.resize(bytes());
				uint8_t *ptr = (uint8_t*)buffer.data();
				endec::put_uint64(ptr, index_);
				endec::put_uint64(ptr, term_);
				endec::put_uint8(ptr, static_cast<uint8_t>(type_));
				endec::put_string(ptr, log_data_);
				return buffer;
			}
			bool from_string(const std::string &buffer)
			{
				uint8_t *ptr = (uint8_t*)buffer.data();
				log_data_.clear();
				if (buffer.size() < bytes())
					return false;
				index_ = (int64_t)endec::get_uint64(ptr);
				term_ = (int64_t)endec::get_uint64(ptr);
				type_ = (type)endec::get_uint8(ptr);
				log_data_ = endec::get_string(ptr);
				return true;
			}
			bool from_string(uint8_t *&ptr)
			{
				log_data_.clear();
				index_ = (int64_t)endec::get_uint64(ptr);
				term_ = (int64_t)endec::get_uint64(ptr);
				type_ = (type)endec::get_uint8(ptr);
				log_data_ = endec::get_string(ptr);
				return true;
			}
		};
		struct raft_config
		{
			struct raft_node
			{
				raft_node() {}
				raft_node(const raft_node &) = default;
				raft_node(
					std::string ip,
					int port,
					std::string raft_id)
				{
					ip_ = ip;
					port_ = port;
					raft_id_ = raft_id;
				}
				std::string ip_;
				int port_ = 0;
				std::string raft_id_;
			};
			using nodes = std::vector<raft_node>;
			nodes peers_;
			raft_node myself_;
			std::size_t raft_log_size_ = 0;
			std::size_t raft_log_count_ = 0;
			std::size_t append_log_timeout_ = 0;
			std::size_t election_timeout_ = 0;
			std::size_t heartbeat_interval_ = 0;
			std::string raftlog_base_path_;
			std::string snapshot_base_path_;
			std::string metadata_base_path_;
		};
		struct append_entries_request
		{
			int64_t term_ = 0;
			std::string leader_id_;
			int64_t prev_log_index_ = 0;
			int64_t prev_log_term_ = 0;
			int64_t leader_commit_ = 0;
			std::list<log_entry>entries_;

			XENDEC(term_, 
				leader_id_, 
				prev_log_index_, 
				prev_log_term_, 
				leader_commit_, 
				entries_);
		};

		struct append_entries_response
		{
			int64_t term_ = 0;
			int64_t last_log_index_ = 0;
			bool success_ = false;

			XENDEC(term_, last_log_index_, success_);
		};

		struct install_snapshot_request
		{
			int64_t term_ = 0;
			int64_t last_snapshot_index_ = 0;
			int64_t last_included_term_ = 0;
			int64_t offset_ = 0;
			bool done_ = false;
			std::string leader_id_;
			std::string data_;

			XENDEC(term_, 
				last_snapshot_index_, 
				last_included_term_, 
				offset_, 
				done_, 
				leader_id_, 
				data_);
		};

		struct install_snapshot_response
		{
			int64_t term_ = 0;
			int64_t bytes_stored_ = 0;

			XENDEC(term_, bytes_stored_);
		};
	}
}