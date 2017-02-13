#pragma once
namespace xmqtt
{
	namespace msg_packer
	{
		inline char* encode_varint32(unsigned char* dst, uint32_t v)
		{
			if (v < (1 << 7))
				*(dst++) = v;
			else if (v < (1 << 14)) 
			{
				*(dst++) = v | 0x80;
				*(dst++) = v >> 7;
			}
			else if (v < (1 << 21)) 
			{
				*(dst++) = v | 0x80;
				*(dst++) = (v >> 7) | 0x80;
				*(dst++) = v >> 14;
			}
			else if (v < (1 << 28)) 
			{
				*(dst++) = v | 0x80;
				*(dst++) = (v >> 7) | 0x80;
				*(dst++) = (v >> 14) | 0x80;
				*(dst++) = v >> 21;
			}
			else
				return (char *)dst;//too big

			return reinterpret_cast<char*>(dst);
		}

		static inline int copy_data(unsigned char *ptr, const std::string &str)
		{
			ptr[0] = (uint8_t)(str.size() >> 8) & 0x00ff;
			ptr[1] = (uint8_t)(str.size() & 0x00ff);
			if (str.size())
				memcpy(ptr + 2, str.data(), str.size());
			return  2 + str.size();
		}
		static inline std::string packet_msg(const proto::connect_msg &msg)
		{
			int remaining_length = msg.protocol_name_.size() +
				2 +//protocol_name_
				1 +//protocol_level_
				1 +//connect_flags_
				2 +//keep_alive_
				msg.clientid_.size() + 2 +//clientId
				(msg.connect_flags_.password_flag_ ? msg.password_.size() + 2 : 0) +
				(msg.connect_flags_.user_name_flag_ ? msg.user_name_.size() + 2 : 0) +
				(msg.connect_flags_.will_flag_ ? msg.will_topic_.size() + 2 : 0) +
				(msg.connect_flags_.will_flag_ ? msg.will_message_.size() + 2 : 0);

			std::string data;
			data.resize(remaining_length + 5);

			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::packet_type::e_connect;
			auto p = encode_varint32(ptr + 1, remaining_length);
			int offset = p - data.data();
			offset += copy_data(ptr + offset, msg.protocol_name_);
			ptr[offset] = msg.protocol_level_;
			offset++;

			ptr[offset] |= msg.connect_flags_.user_name_flag_ ? 0x80 : 0x00;
			ptr[offset] |= msg.connect_flags_.password_flag_ ? 0x40 : 0x00;
			ptr[offset] |= msg.connect_flags_.will_retain_ ? 0x20 : 0x00;
			ptr[offset] |= msg.connect_flags_.will_qoS_ ? 0x18 : 0x00;
			ptr[offset] |= msg.connect_flags_.will_flag_ ? 0x04 : 0x00;
			ptr[offset] |= msg.connect_flags_.clean_session_ ? 0x02 : 0x00;
			offset++;

			ptr[offset] = msg.keep_alive_ >> 8;
			ptr[offset + 1] = msg.keep_alive_ && 0x00ff;
			offset += 2;

			offset += copy_data(ptr + offset, msg.clientid_);
			if (msg.connect_flags_.will_flag_) 
			{
				offset += copy_data(ptr + offset, msg.will_topic_);
				offset += copy_data(ptr + offset, msg.will_message_);
			}
			if (msg.connect_flags_.user_name_flag_)
				offset += copy_data(ptr + offset, msg.user_name_);
			if (msg.connect_flags_.password_flag_)
				offset += copy_data(ptr + offset, msg.password_);

			data.resize(offset);
			return data;
		}

		std::string packet_msg(const proto::connect_ack_msg &msg)
		{
			std::string data;
			data.resize(4);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_connect_ack;
			ptr[1] = 0x02;
			ptr[2] = msg.session_present_flag_ & 0x01;
			ptr[3] = msg.return_code_;
			return data;
		}

		std::string packet_msg(const proto::subscribe_msg &msg)
		{
			auto remaining_length = 2;
			for (auto &itr : msg.topic_filters)
			{
				remaining_length += 2;
				remaining_length += itr.topic_.size();
				remaining_length += 1;
			}

			std::string data;
			data.resize(remaining_length + 4);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_subscribe | 0x02;
			auto p = encode_varint32(ptr + 1, remaining_length);
			auto offset = p - data.data();
			ptr[offset] = msg.packet_identifier_ >> 8;
			ptr[offset + 1] |= msg.packet_identifier_ & 0x00ff;
			offset += 2;

			for (auto &itr : msg.topic_filters) 
			{
				offset += copy_data(ptr + offset, itr.topic_);
				ptr[offset] = itr.requested_QoS_;
				offset++;
			}
			data.resize(offset);
			return data;
		}

		std::string packet_msg(const proto::unsubscribe_msg &msg)
		{
			std::string data;
			int remaining_length = 2;
			
			for (auto &itr : msg.topic_filters_) 
			{
				remaining_length += itr.size();
				remaining_length += 2;
			}
			
			data.resize(remaining_length + 4);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_unsubscribe;
			auto p = encode_varint32(ptr + 1, remaining_length);
			auto offset = p - data.data();
			ptr[offset] = msg.packet_identifier_ >> 8;
			ptr[offset + 1] |= msg.packet_identifier_ & 0x00ff;
			offset += 2;

			for (auto &itr : msg.topic_filters_) 
				offset += copy_data(ptr + offset, itr);

			data.resize(offset);
			return data;
		}

		inline std::string packet_msg(const proto::subscribe_ack_msg &msg)
		{
			int remaining_length = 2;
			remaining_length += msg.return_code_.size();
			std::string data;
			data.resize(remaining_length + 4);
			unsigned char* ptr = (unsigned char*)(data.data());

			ptr[0] = proto::e_subscribe_ack;
			auto p = encode_varint32(ptr + 1, remaining_length);
			auto offset = p - data.data();

			ptr[offset] = msg.packet_identifier_ >> 8;
			ptr[offset + 1] = msg.packet_identifier_ & 0x00ff;
			offset += 2;

			for (auto &it : msg.return_code_) 
			{
				ptr[offset] = it;
				++offset;
			}
			data.resize(offset);
			return data;
		}

		inline std::string packet_msg(const proto::publish_msg &msg)
		{
			std::string data;
			uint32_t remaining_length =
				(msg.payload_ ? (uint32_t)msg.payload_->size() : 0) +
				(msg.fixed_header_.QoS_level_ ? 2 : 0) +
				(2 + (uint32_t)msg.topic_name_.size());

			data.resize(remaining_length + 5);

			unsigned char* ptr = (unsigned char*)(data.data());

			ptr[0] = proto::e_publish;
			ptr[0] |= msg.fixed_header_.DUP_flag_ ? 0x08 : 0;
			ptr[0] |= msg.fixed_header_.QoS_level_ << 1;
			ptr[0] |= msg.fixed_header_.RETAIN_ ? 0x01 : 0;

			auto p = encode_varint32(ptr + 1, remaining_length);
			auto offset = p - data.data();
			offset += copy_data(ptr + offset, msg.topic_name_);
			
			if (msg.fixed_header_.QoS_level_) 
			{
				ptr[offset] = msg.packet_identifier_ >> 8;
				ptr[offset + 1] = msg.packet_identifier_ & 0x00ff;
				offset += 2;
			}

			if (msg.payload_->size()) 
			{
				memcpy(ptr + offset, msg.payload_->data(), msg.payload_->size());
				offset += msg.payload_->size();
			}
			assert((int)data.size() >= offset);
			data.resize(offset);
			return data;
		}

		inline std::string packet_msg(const proto::ping_req_msg &msg)
		{
			std::string data;
			data.resize(2);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_ping_req;
			ptr[1] = 0;
			return data;
		}

		inline std::string packet_msg(const proto::ping_rsp_msg &msg)
		{
		
			std::string data;
			data.resize(2);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_ping_rsp;
			ptr[1] = 0;
			return data;
		}

		inline std::string packet_msg(const proto::publish_ack_msg &msg)
		{
			std::string data;
			data.resize(4);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_publish_ack;
			ptr[1] = 0x02;
			ptr[2] = msg.packet_identifier_ >> 8;
			ptr[3] = msg.packet_identifier_ & 0x00ff;
			return data;
		}

		inline std::string packet_msg(const proto::publish_rec_msg &msg)
		{
			std::string data;
			data.resize(4);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_publish_rec;
			ptr[1] = 0x02;
			ptr[2] = msg.packet_identifier_ >> 8;
			ptr[3] = msg.packet_identifier_ & 0x00ff;
			return data;
		}

		inline std::string packet_msg(const proto::publish_rel_msg &msg)
		{
			std::string data;
			data.resize(4);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_publish_rel | 0x02;
			ptr[1] = 0x02;
			ptr[2] = msg.packet_identifier_ >> 8;
			ptr[3] = msg.packet_identifier_ & 0x00ff;
			return data;
		}

		inline std::string packet_msg(const proto::publish_comp_msg &msg)
		{
			std::string data;
			data.resize(4);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_publish_comp;
			ptr[1] = 0x02;
			ptr[2] = msg.packet_identifier_ >> 8;
			ptr[3] = msg.packet_identifier_ & 0x00ff;
			return data;
		}

		inline std::string packet_msg(const proto::unsubscribe_ack_msg &msg)
		{
			std::string data;
			data.resize(4);
			unsigned char* ptr = (unsigned char*)(data.data());
			ptr[0] = proto::e_unsubscribe_ack;
			ptr[1] = 0x02;
			ptr[2] = msg.packet_identifier_ >> 8;
			ptr[3] = msg.packet_identifier_ & 0x00ff;
			return data;
		}
	}
}