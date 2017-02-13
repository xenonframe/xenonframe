#pragma once
namespace xmqtt
{

	class packet_parser
	{
	public:
		typedef std::function<bool(proto::disconnect_msg &&)>		disconnect_msg_handler_t;
		typedef std::function<bool(proto::ping_rsp_msg &&)>			ping_rsp_msg_handler_t;
		typedef std::function<bool(proto::ping_req_msg &&)>			ping_req_msg_handler_t;
		typedef std::function<bool(proto::unsubscribe_ack_msg &&)>	unsubscribe_ack_msg_handler_t;
		typedef std::function<bool(proto::unsubscribe_msg &&)>		unsubscribe_msg_handler_t;
		typedef std::function<bool(proto::subscribe_ack_msg &&)>	subscribe_ack_msg_handler_t;
		typedef std::function<bool(proto::subscribe_msg &&)>		subscribe_msg_handler_t;
		typedef std::function<bool(proto::publish_comp_msg &&)>		publish_comp_msg_handler_t;
		typedef std::function<bool(proto::publish_rel_msg &&)>		publish_rel_msg_handler_t;
		typedef std::function<bool(proto::publish_rec_msg &&)>		publish_rec_msg_handler_t;
		typedef std::function<bool(proto::publish_msg&&)>			publish_msg_handler_t;
		typedef std::function<bool(proto::publish_ack_msg &&)>		publish_ack_msg_handler_t;
		typedef std::function<bool(proto::connect_msg &&)>			connect_msg_handler_t;
		typedef std::function<bool(proto::connect_ack_msg &&)>		connect_ack_msg_handler_t;


		packet_parser()
		{
		}

		packet_parser(packet_parser && _packet_parser)
		{
			operator=(std::move(_packet_parser));
		}

		packet_parser &operator= (packet_parser && _packet_parser)
		{
			buffer_ = std::move(_packet_parser.buffer_);
			buffer_len_ = _packet_parser.buffer_len_;
			variant32_shift_ = _packet_parser.variant32_shift_;
			fixed_header_ = _packet_parser.fixed_header_;
			step_ = _packet_parser.step_;

			_packet_parser.reset();
			return *this;
		}
		~packet_parser()
		{
		}

		bool do_parse(const char *data, int len)
		{
			assert(data);
			int pos = 0;
			do {
				switch (step_)
				{
				case packet_parser::e_fixed_header_type:
				case packet_parser::e_fixed_header_remaining_length:
					pos += parse_fixed_header(data + pos, len - pos);
					break;
				case packet_parser::e_get_remaining_data:
					pos += store_remianing_data(data + pos, len - pos);
					if (step_ == e_get_remaining_data_done)
					{
						step_ = e_fixed_header_type;
						if (parse_msg() == false)
							return false;
					}
					break;
				default:
					assert(false);
					break;
				}
			} while (pos < len);
			return status_;
		}

		packet_parser &bind_connect_msg_handler(const connect_msg_handler_t &handle)
		{
			connect_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_connect_ack_msg_handler(const connect_ack_msg_handler_t& handle)
		{
			connect_ack_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_publish_msg_handler(const publish_msg_handler_t &handle)
		{
			publish_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_publish_ack_msg_handler(const publish_ack_msg_handler_t & handle)
		{
			publish_ack_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_publish_rec_msg_handler(const publish_rec_msg_handler_t& handle)
		{
			publish_rec_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_publish_rel_msg_handler(const publish_rel_msg_handler_t &handle)
		{
			publish_rel_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_publish_comp_msg_handler(const publish_comp_msg_handler_t& handle)
		{
			publish_comp_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_subscribe_msg_handler(const subscribe_msg_handler_t &handle)
		{
			subscribe_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_subscribe_ack_msg_handler(const subscribe_ack_msg_handler_t handle)
		{
			subscribe_ack_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_unsubscribe_msg_handler(const unsubscribe_msg_handler_t& handle)
		{
			unsubscribe_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_unsubscribe_ack_msg_handler(const unsubscribe_ack_msg_handler_t& handle)
		{
			unsubscribe_ack_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_ping_req_msg_handler(const ping_req_msg_handler_t& handle)
		{
			ping_req_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_ping_rsp_msg_handler(const ping_rsp_msg_handler_t& handle)
		{
			ping_rsp_msg_handler_ = handle;
			return *this;
		}

		packet_parser &bind_disconnect_msg_handler(const disconnect_msg_handler_t& handle)
		{
			disconnect_msg_handler_ = handle;
			return *this;
		}

		void reset()
		{
			buffer_.clear();
			buffer_len_ = 0;
			step_ = e_fixed_header_type;
			variant32_shift_ = 0;
			fixed_header_.reset();
		}
	private:
		packet_parser(const packet_parser &)
		{

		}
		packet_parser &operator= (const packet_parser &)
		{
			return *this;
		}

		uint16_t ntoh16(const char *buffer)
		{
			return (((uint16_t)buffer[0] & 0x00ff) << 8) | ((uint16_t)buffer[1] & 0x00ff);
		}

		int parse_fixed_header(const char * data, int len)
		{
			int pos = 0;

			if (step_ == e_fixed_header_type)
			{
				fixed_header_.reset();
				fixed_header_.packet_type_ = (proto::packet_type)(*(uint8_t*)data & 0xf0);
				fixed_header_.reserved_ = ((*(uint8_t*)data) & 0x0f);
				fixed_header_.DUP_flag_ = (*(uint8_t*)data) & 0x08;
				fixed_header_.QoS_level_ = ((*(uint8_t*)data) & 0x06) >> 1;
				fixed_header_.RETAIN_ = (*(uint8_t*)data) & 0x01;
				step_ = e_fixed_header_remaining_length;
				pos += 1;
				if (pos == len) return pos;
			}

			if (step_ == e_fixed_header_remaining_length)
			{
				uint32_t value = 0;
				do
				{
					value = *(uint8_t*)(data + pos);
					if (value & 0x80) {
						fixed_header_.remaining_length_
							|= value & 0x7f << variant32_shift_;
						variant32_shift_ += 7;//next
					}
					else {
						fixed_header_.remaining_length_
							|= value << variant32_shift_;
						step_ = e_get_remaining_data;
						buffer_.clear();
						buffer_len_ = fixed_header_.remaining_length_;
						if (buffer_len_)
							buffer_.reserve(buffer_len_);
						variant32_shift_ = 0;
						++pos;
						break;
					}
					++pos;
					if (pos == len) return pos;

				} while (value & 0x80 && variant32_shift_ < 21);
			}
			if (fixed_header_.remaining_length_ == 0) {
				status_ = parse_msg();
				step_ = e_fixed_header_type;
			}
			return pos;
		}

		int store_remianing_data(const char *data, int len)
		{
			uint32_t min_len = std::min<uint32_t>(len, buffer_len_ - buffer_.size());

			buffer_.append(data, min_len);
			if (buffer_.size() == buffer_len_)
				step_ = e_get_remaining_data_done;
			return min_len;
		}

		void reset_buffer_status()
		{
			buffer_.clear();
			buffer_len_ = 0;
		}

		bool parse_msg()
		{
			switch (fixed_header_.packet_type_)
			{

			case proto::e_connect:
				return parse_connect_msg();
			case proto::e_connect_ack:
				return parse_connect_ack_msg();
			case proto::e_publish:
				return parse_publish_msg();
			case proto::e_publish_ack:
				return parse_publish_ack_msg();
			case proto::e_publish_rec:
				return parse_publish_rec_msg();
			case proto::e_publish_rel:
				return parse_publish_rel_msg();
			case proto::e_publish_comp:
				return parse_pushlish_comp_msg();
			case proto::e_subscribe:
				return parse_subscribe_msg();
			case proto::e_subscribe_ack:
				return parse_subscribe_ack_msg();
			case proto::e_unsubscribe:
				return parse_unsubscribe_msg();
			case proto::e_unsubscribe_ack:
				return parse_unsubscribe_ack_msg();
			case proto::e_ping_req:
				return parse_ping_req_msg();
			case proto::e_ping_rsp:
				return parse_ping_rsp_msg();
			case proto::e_disconnect:
				return parse_disconnect_msg();
			default:
				return false;
			}
		}

		#define check_remain_len(val) \
		do{ \
			if(offset + val > len)\
			return false;\
		 }while(0);

		bool parse_connect_msg()
		{
			proto::connect_msg msg;
			const char *ptr = buffer_.c_str();
			int offset = 0;
			int len;

			msg.fixed_header_ = fixed_header_;
			len = msg.fixed_header_.remaining_length_;


			check_remain_len(2);
			uint16_t protocol_name_len = ntoh16(ptr + offset);
			offset += 2;
			if (!protocol_name_len)
				return false;
			check_remain_len(protocol_name_len);
			msg.protocol_name_.append(ptr + offset, protocol_name_len);
			offset += protocol_name_len;

			check_remain_len(1);
			msg.protocol_level_ = *(uint8_t*)(ptr + offset);
			offset += 1;
			//for MQTT 3.1
			if (msg.fixed_header_.QoS_level_ == 0x01 &&
				msg.protocol_level_ != 0x03)
				return false;

			check_remain_len(1);
			msg.connect_flags_.user_name_flag_ = *(uint8_t*)(ptr + offset) & 0x80;
			msg.connect_flags_.password_flag_ = *(uint8_t*)(ptr + offset) & 0x40;
			msg.connect_flags_.will_retain_ = *(uint8_t*)(ptr + offset) & 0x20;
			msg.connect_flags_.will_qoS_ = (*(uint8_t*)(ptr + offset) & 0x18) >> 3; //0001 1000
			msg.connect_flags_.will_flag_ = *(uint8_t*)(ptr + offset) & 0x04;
			msg.connect_flags_.clean_session_ = *(uint8_t*)(ptr + offset) & 0x02;
			msg.connect_flags_.reserved_ = *(uint8_t*)(ptr + offset) & 0x01;
			offset += 1;

			check_remain_len(2);
			msg.keep_alive_ = *(uint8_t*)(ptr + offset) << 8;
			msg.keep_alive_ += *(uint8_t*)(ptr + offset + 1);
			offset += 2;

			check_remain_len(2);
			uint16_t clientid_len = ntoh16(ptr + offset);
			offset += 2;

			if (clientid_len) 
			{
				check_remain_len(clientid_len);
				msg.clientid_.append(ptr + offset, clientid_len);
				offset += clientid_len;
			}

			if (msg.connect_flags_.will_flag_) 
			{
				check_remain_len(2);
				uint16_t will_topic_len = ntoh16(ptr + offset);
				offset += 2;
				if (!will_topic_len)
					return false;

				check_remain_len(will_topic_len);
				msg.will_topic_.append(ptr + offset, will_topic_len);
				offset += will_topic_len;
				if (check_utf8(msg.will_topic_) == false)
					return false;

				check_remain_len(2);
				uint16_t will_message_len = ntoh16(ptr + offset);
				offset += 2;
				if (!will_message_len)
					return false;

				check_remain_len(will_message_len);
				msg.will_message_.append(ptr + offset, will_message_len);
				offset += will_message_len;
			}

			if (msg.connect_flags_.user_name_flag_) 
			{
				check_remain_len(2);
				uint16_t username_len = ntoh16(ptr + offset);
				offset += 2;
				if (!username_len)
					return false;

				check_remain_len(username_len);
				msg.user_name_.append(ptr + offset, username_len);
				offset += username_len;
			}

			if (msg.connect_flags_.password_flag_) 
			{
				check_remain_len(2);
				uint16_t password_len = ntoh16(ptr + offset);
				offset += 2;

				if (!password_len)
					return false;
				check_remain_len(password_len);
				msg.password_.append(ptr + offset, password_len);
				offset += password_len;
			}

			if (offset != (int)fixed_header_.remaining_length_)
				return false;

			if (connect_msg_handler_)
				return connect_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_connect_ack_msg()
		{
			proto::connect_ack_msg msg;
			msg.fixed_header_ = fixed_header_;

			if (msg.fixed_header_.remaining_length_ != 2)
				return false;

			msg.session_present_flag_ = (*(uint8_t*)buffer_.c_str() & 0x01);
			msg.return_code_ = *(uint8_t*)(buffer_.c_str() + 1);

			if (connect_ack_msg_handler_)
				return connect_ack_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_publish_msg()
		{
			proto::publish_msg msg;

			const char *ptr = buffer_.c_str();
			int offset = 0;
			int len = fixed_header_.remaining_length_;

			msg.fixed_header_ = fixed_header_;
			check_remain_len(2);
			uint16_t topic_name_len = ntoh16(ptr + offset);
			offset += 2;
			if (!topic_name_len)
				return false;

			check_remain_len(topic_name_len);
			msg.topic_name_.append(ptr + offset, topic_name_len);
			offset += topic_name_len;
			if (!check_utf8(msg.topic_name_))
				return false;

			//1 or 2
			if (msg.fixed_header_.QoS_level_) 
			{
				check_remain_len(2);
				msg.packet_identifier_ = ntoh16(ptr + offset);
				offset += 2;
			}

			int payload_len = len - offset;
			if (payload_len > 0) 
			{
				msg.payload_.reset(new std::string);
				msg.payload_->reserve(payload_len);
				msg.payload_->append(ptr + offset, payload_len);
			}
			else if (payload_len < 0)
				return false;

			if (publish_msg_handler_)
				return publish_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_publish_ack_msg()
		{
			proto::publish_ack_msg msg;
			msg.fixed_header_ = fixed_header_;
			if (msg.fixed_header_.reserved_ ||
				msg.fixed_header_.remaining_length_ != 2)
				return false;
			msg.packet_identifier_ = ntoh16(buffer_.c_str());
			if (publish_ack_msg_handler_)
				return publish_ack_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_publish_rec_msg()
		{
			proto::publish_rec_msg msg;
			msg.fixed_header_ = fixed_header_;
			if (msg.fixed_header_.reserved_ ||
				msg.fixed_header_.remaining_length_ != 2)
				return false;
			msg.packet_identifier_ = ntoh16(buffer_.c_str());
			if (publish_rec_msg_handler_)
				return publish_rec_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_publish_rel_msg()
		{
			proto::publish_rel_msg msg;
			msg.fixed_header_ = fixed_header_;
			if (msg.fixed_header_.reserved_ != 0x02 ||
				msg.fixed_header_.remaining_length_ != 0x02)
				return false;
			msg.packet_identifier_ = ntoh16(buffer_.c_str());
			if (publish_rel_msg_handler_)
				return publish_rel_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_pushlish_comp_msg()
		{
			proto::publish_comp_msg msg;
			msg.fixed_header_ = fixed_header_;
			if (msg.fixed_header_.reserved_ ||
				msg.fixed_header_.remaining_length_ != 2)
				return false;
			msg.packet_identifier_ = ntoh16(buffer_.c_str());
			if (publish_comp_msg_handler_)
				return publish_comp_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_subscribe_msg()
		{
			proto::subscribe_msg msg;
			const char *ptr = buffer_.c_str();
			int len = fixed_header_.remaining_length_;
			int offset = 0;

			msg.fixed_header_ = fixed_header_;
			if (msg.fixed_header_.QoS_level_ != 0x01)
				return false;
			check_remain_len(2);
			msg.packet_identifier_ = ntoh16(ptr + offset);
			offset += 2;

			while (offset < len) 
			{
				check_remain_len(2);
				uint16_t topic_filter_len = ntoh16(ptr + offset);
				offset += 2;
				if (!topic_filter_len)
					return false;
				check_remain_len(topic_filter_len);
				std::string topic_filter(ptr + offset, topic_filter_len);
				offset += topic_filter_len;
				if (check_utf8(topic_filter) == false)
					return false;

				check_remain_len(1);
				uint8_t requested_QoS = *(uint8_t*)(ptr + offset);
				offset += sizeof(uint8_t);
				if (requested_QoS & 0xfc) //1111 1100
					return false;

				msg.topic_filters.emplace_back(
					std::move(topic_filter), requested_QoS);
			}
			if (msg.topic_filters.empty())
				return false;
			if (subscribe_msg_handler_)
				return subscribe_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_subscribe_ack_msg()
		{
			proto::subscribe_ack_msg msg;
			const char *ptr = buffer_.c_str();
			int len = fixed_header_.remaining_length_;
			int offset = 0;

			msg.fixed_header_ = fixed_header_;
			if (msg.fixed_header_.reserved_)
				return false;
			check_remain_len(2);
			msg.packet_identifier_ = ntoh16(ptr);
			offset += 2;

			do 
			{
				check_remain_len(1);
				msg.return_code_.push_back(*ptr);
				offset += 1;
			} while (offset < len);

			if (subscribe_ack_msg_handler_)
				return subscribe_ack_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_unsubscribe_msg()
		{
			proto::unsubscribe_msg msg;
			const char *ptr = buffer_.c_str();
			int len = fixed_header_.remaining_length_;
			int offset = 0;

			msg.fixed_header_ = fixed_header_;
			if (msg.fixed_header_.reserved_)
				return false;

			check_remain_len(2);
			msg.packet_identifier_ = ntoh16(ptr + offset);
			offset += 2;

			while (offset < len)
			{
				check_remain_len(2);
				uint16_t topic_fiter_len = ntoh16(ptr + offset);
				offset += 2;
				if (!topic_fiter_len)
					return false;
				check_remain_len(topic_fiter_len);
				msg.topic_filters_.emplace_back(ptr + offset, topic_fiter_len);
				if (!check_utf8(msg.topic_filters_.back()))
					return false;
				offset += topic_fiter_len;
			}

			if (unsubscribe_msg_handler_)
				return unsubscribe_msg_handler_(std::move(msg));
			return false;

		}

		bool parse_unsubscribe_ack_msg()
		{
			proto::unsubscribe_ack_msg msg;
			msg.fixed_header_ = fixed_header_;
			if (msg.fixed_header_.reserved_ ||
				fixed_header_.remaining_length_ != 2)
				return false;
			msg.packet_identifier_ = ntoh16(buffer_.c_str());
			if (unsubscribe_ack_msg_handler_)
				return unsubscribe_ack_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_ping_req_msg()
		{
			proto::ping_req_msg msg;
			msg.fixed_header_ = fixed_header_;
			if (fixed_header_.reserved_ ||
				fixed_header_.remaining_length_)
				return false;
			if (ping_req_msg_handler_)
				return ping_req_msg_handler_(std::move(msg));
			return false;
		}


		bool parse_ping_rsp_msg()
		{
			proto::ping_rsp_msg msg;
			msg.fixed_header_ = fixed_header_;
			if (fixed_header_.remaining_length_ ||
				fixed_header_.reserved_)
				return false;
			if (ping_rsp_msg_handler_)
				return ping_rsp_msg_handler_(std::move(msg));
			return false;
		}

		bool parse_disconnect_msg()
		{
			proto::disconnect_msg msg;
			msg.fixed_header_ = fixed_header_;
			if (fixed_header_.reserved_ || fixed_header_.remaining_length_)
				return false;
			if (disconnect_msg_handler_)
				return disconnect_msg_handler_(std::move(msg));
			return false;
		}

	private:
		enum status_t
		{
			e_fixed_header_type,
			e_fixed_header_remaining_length,
			e_get_remaining_data,
			e_get_remaining_data_done,
		};

		disconnect_msg_handler_t			disconnect_msg_handler_;
		ping_rsp_msg_handler_t				ping_rsp_msg_handler_;
		ping_req_msg_handler_t				ping_req_msg_handler_;
		unsubscribe_ack_msg_handler_t		unsubscribe_ack_msg_handler_;
		unsubscribe_msg_handler_t			unsubscribe_msg_handler_;
		subscribe_ack_msg_handler_t			subscribe_ack_msg_handler_;
		subscribe_msg_handler_t				subscribe_msg_handler_;
		publish_comp_msg_handler_t			publish_comp_msg_handler_;
		publish_rel_msg_handler_t			publish_rel_msg_handler_;
		publish_rec_msg_handler_t			publish_rec_msg_handler_;
		publish_msg_handler_t				publish_msg_handler_;
		publish_ack_msg_handler_t			publish_ack_msg_handler_;
		connect_msg_handler_t				connect_msg_handler_;
		connect_ack_msg_handler_t			connect_ack_msg_handler_;

		proto::fixed_header					fixed_header_;
		std::string							buffer_;
		int									buffer_len_ = 0;
		int									variant32_shift_ = 0; //
		status_t							step_ = e_fixed_header_type;
		bool								status_ = true;
	};
}