#pragma once
namespace xsocket_io
{
	namespace detail
	{
		enum packet_type
		{
			e_null0 = 100,
			e_open = 0,
			e_close,
			e_ping,
			e_pong,
			e_message,
			e_upgrade,
			e_noop
		};
		enum playload_type
		{
			e_null1 = 100,
			e_connect = 0,
			e_disconnect,
			e_event,
			e_ack,
			e_error,
			e_binary_event,
			e_binary_ack
		};
		enum class error_msg
		{
			e_transport_unknown,
			e_session_id_unknown,
			e_bad_handshake_method,
			e_bad_request,
		};
		struct packet
		{
			std::string playload_;
			packet_type packet_type_ = packet_type::e_null0;
			playload_type playload_type_ = playload_type::e_null1;
			int64_t id_ = 0;
			std::string nsp_= "/";
			bool binary_ = false;
			bool is_string_ = true;;
		};
		inline std::string to_json(error_msg msg)
		{
			if (msg == error_msg::e_bad_handshake_method)
				return "{\"code\":2, ""\"message\":\"Bad handshake method\"}";
			else if (msg == error_msg::e_bad_request)
				return "{\"code\":3, \"message\":\"Bad request\"}";
			else if (msg == error_msg::e_session_id_unknown)
				return "{\"code\":1, \"message\":\"Session ID unknown\"}";
			else if (msg == error_msg::e_transport_unknown)
				return "{\"code\":0, \"message\":\"Transport unknown\"}";
			assert(false);
			return{};
		}
		inline std::string encode_packet(const packet &_packet, bool ws = false)
		{
			auto nsp = false;
			std::string playload = std::to_string(_packet.packet_type_);

			if (_packet.playload_type_ != playload_type::e_null1)
			{
				playload += std::to_string(_packet.playload_type_);
			}
			if (_packet.packet_type_ == e_message && _packet.nsp_ != "/")
			{
				playload += _packet.nsp_;
				nsp = true;
			}
			if (_packet.id_ != 0)
			{
				nsp = false;
				playload.push_back(',');
				playload.append(std::to_string(_packet.id_));
			}
			if (_packet.playload_.size())
			{
				if (nsp)
					playload.push_back(',');
				playload += _packet.playload_;
			}
			if (ws)
				return playload;

			if (!_packet.binary_)
			{
				return std::to_string(_packet.playload_.size()) + ":" + _packet.playload_;
			}
			
			auto playload_len = std::to_string(playload.size());
			std::string buffer;
			buffer.resize(playload_len.size() + 2);

			buffer[0] = 0;
			if (!_packet.is_string_)
				buffer[0] = 1;

			for (size_t i = 0; i < playload_len.size(); i++)
			{
				std::string temp;
				temp.push_back(playload_len[i]);
				auto num = std::strtol(temp.c_str(), 0, 10);
				buffer[i + 1] = ((char)num);
			}
			buffer[buffer.size() - 1] = (char)255;
			buffer.append(playload);
			return buffer;
		}

		
		inline std::list<packet> decode_packet(const std::string &data,bool binary, bool ws = false)
		{

			#define assert_ptr()\
			if(pos >= end) \
				throw packet_error("parse packet error");

			std::list<packet> packets;

			auto pos = (char*)data.c_str();
			auto end = pos + data.size();
			do
			{
				packet _packet;
				packet_type _packet_type = e_null0;
				playload_type _playload_type = e_null1;
				long len = (long)data.size();
				if (!ws)
				{
					if (!binary)
					{
						char *ptr = nullptr;
						len = std::strtol(pos, &ptr, 10);
						if (!len || !ptr || (*ptr != ':'))
							throw packet_error("parse packet len error");
						++ptr;
						assert_ptr();
						pos = ptr;
					}
					else
					{
						std::string len_str;
						if (*pos != 0 && *pos != 1)
							throw packet_error("packet error");
						_packet.is_string_ = *pos == 0;
						++pos;
						do
						{
							len_str.push_back(*pos);
							++pos;
							assert_ptr();
						} while (*pos != 255);

						auto len = std::strtol(len_str.c_str(), nullptr, 10);
						if (!len)
							throw packet_error("packet len error");
						++pos;
						assert_ptr();
					}
				}

				char ch = *pos - '0';
				if (ch < e_open || ch > e_noop)
					throw packet_error("parse packet type error");
				_packet_type = static_cast<packet_type>(ch);
				++pos;
				len--;
				if (len > 0)
				{
					if (_packet_type == e_ping)
					{
						_packet.playload_ = data.substr(pos - data.c_str(), len);
						pos += len;
					}
					else
					{
						assert_ptr();
						ch = *pos - '0';
						if (ch < e_connect || ch > e_binary_ack)
							throw packet_error("pakcet playload_type error");
						_playload_type = static_cast<playload_type>(ch);
						++pos;
						len--;
						if (len > 0)
						{
							assert_ptr();
							//parse nsp
							if (*pos == '/')
							{
								_packet.nsp_.clear();
								do
								{
									--len;
									if (*pos != ',')
										_packet.nsp_.push_back(*pos);
									else
									{
										++pos;
										break;
									}
									++pos;
								} while (pos < end);
							}
							//parse packet id
							if (pos < end && isdigit(*pos))
							{
								std::string id;
								do
								{
									id.push_back(*pos);
									--len;
									++pos;
								} while (pos < end && isdigit(*pos));
								_packet.id_ = std::strtoll(id.c_str(), 0, 10);
							}
							if (len < 0)
								throw packet_error("packet len error");
							if (len > 0)
							{
								_packet.playload_.append(pos, len);
								pos += len;
							}
						}
					}
				}
				_packet.binary_ = binary;
				_packet.packet_type_ = _packet_type;
				_packet.playload_type_ = _playload_type;
				packets.emplace_back(std::move(_packet));
			} while (pos < end);

			return packets;
		}
		struct open_msg 
		{
			std::string sid;
			std::vector<std::string> upgrades;
			int64_t pingInterval;
			int64_t pingTimeout;
			XGSON(sid, upgrades, pingInterval, pingTimeout);
		};


	}
}