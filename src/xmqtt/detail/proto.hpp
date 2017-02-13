#pragma once
namespace xmqtt
{
	namespace proto
	{
		enum packet_type:uint8_t
		{
			e_null = 0x00,
			e_connect = 0x10,
			e_connect_ack = 0x20,
			e_publish = 0x30,
			e_publish_ack = 0x40,
			e_publish_rec = 0x50,
			e_publish_rel = 0x60,
			e_publish_comp = 0x70,
			e_subscribe = 0x80,
			e_subscribe_ack = 0x90,
			e_unsubscribe = 0xA0,
			e_unsubscribe_ack = 0xB0,
			e_ping_req = 0xC0,
			e_ping_rsp = 0xD0,
			e_disconnect = 0xE0
		};

		struct fixed_header
		{
			packet_type packet_type_ = e_null;
			uint8_t reserved_ = 0;
			uint8_t DUP_flag_ = 0;
			uint8_t QoS_level_ = 0;
			uint8_t RETAIN_ = 0;
			uint32_t remaining_length_ = 0;
			void reset()
			{
				packet_type_ = e_null;
				remaining_length_ = 0;
				DUP_flag_ = 0;
				QoS_level_ = 0;
				RETAIN_ = 0;
				reserved_ = 0;
			}
		};

		struct connect_msg
		{
			fixed_header fixed_header_;
			std::string protocol_name_;
			uint8_t protocol_level_;
			struct connect_flags
			{
				uint8_t user_name_flag_;
				uint8_t password_flag_;
				uint8_t will_retain_;
				uint8_t will_qoS_;
				uint8_t will_flag_;
				uint8_t clean_session_;
				uint8_t reserved_;
			}connect_flags_;
			uint16_t keep_alive_;
			std::string clientid_;
			std::string will_topic_;
			std::string	will_message_;
			std::string user_name_;
			std::string password_;
		};

		struct connect_ack_msg
		{
			fixed_header fixed_header_ = {};
			uint8_t session_present_flag_ = 0;
			uint8_t return_code_ = 0;
		};

		struct publish_msg
		{
			fixed_header fixed_header_;
			std::string topic_name_;
			uint16_t packet_identifier_ = 0;
			std::shared_ptr<std::string> payload_;
		};

		struct publish_ack_msg
		{
			fixed_header fixed_header_;
			uint16_t packet_identifier_;
		};
		
		struct publish_rec_msg
		{
			fixed_header fixed_header_;
			uint16_t packet_identifier_;
		};
		
		struct publish_rel_msg
		{
			fixed_header fixed_header_;
			uint16_t packet_identifier_;
		};

		struct publish_comp_msg
		{
			fixed_header fixed_header_;
			uint16_t packet_identifier_;
		};

		struct subscribe_msg
		{
			fixed_header fixed_header_;
			uint16_t packet_identifier_;
			struct topic_filter
			{
				topic_filter(std::string && _topic_fiter, uint8_t _requested_QoS)
					:topic_(std::move(_topic_fiter)),
					requested_QoS_(_requested_QoS)
				{
				}
				topic_filter()
				{

				}
				std::string topic_;
				uint8_t requested_QoS_;
			};
			std::vector<topic_filter> topic_filters;
		};
		inline bool operator== (const subscribe_msg::topic_filter& lhs,
			const subscribe_msg::topic_filter& rhs) {

			return (lhs.topic_ == rhs.topic_) &&
				(lhs.requested_QoS_ == rhs.requested_QoS_);
		}

		struct subscribe_ack_msg
		{
			fixed_header fixed_header_;
			uint16_t packet_identifier_;
			std::vector<uint8_t> return_code_;
		};

		struct unsubscribe_msg
		{
			fixed_header fixed_header_;
			uint16_t packet_identifier_;
			std::vector<std::string> topic_filters_;
		};

		struct unsubscribe_ack_msg
		{
			fixed_header fixed_header_;
			uint16_t packet_identifier_;
		};

		struct ping_req_msg
		{
			fixed_header fixed_header_;
		};

		struct ping_rsp_msg
		{
			fixed_header fixed_header_;
		};

		struct disconnect_msg
		{
			fixed_header fixed_header_;
		};
	}
}