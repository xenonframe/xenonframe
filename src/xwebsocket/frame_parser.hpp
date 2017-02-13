#pragma once
#include <functional>
#include <stdint.h>
#include <cassert>
#include <algorithm>
#include <iostream>

namespace xwebsocket
{
	/*
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-------+-+-------------+-------------------------------+
	|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
	|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
	|N|V|V|V|       |S|             |   (if payload len==126/127)   |
	| |1|2|3|       |K|             |                               |
	+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
	|     Extended payload length continued, if payload len == 127  |
	+ - - - - - - - - - - - - - - - +-------------------------------+
	|                               |Masking-key, if MASK set to 1  |
	+-------------------------------+-------------------------------+
	| Masking-key (continued)       |          Payload Data         |
	+-------------------------------- - - - - - - - - - - - - - - - +
	:                     Payload Data continued ...                :
	+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
	|                     Payload Data continued ...                |
	+---------------------------------------------------------------+
	*/

	class frame_parser
	{
	public:
		using frame_callback = std::function<void(std::string &&, frame_type, bool)>;

		frame_parser()
		{
			reset();
		}

		~frame_parser()
		{

		}
		frame_parser(frame_parser &&parser)
		{
			move_reset(std::move(parser));
		}
		frame_parser &operator= (frame_parser &&parser)
		{
			move_reset(std::move(parser));
			return *this;
		}
		frame_parser &regist_frame_callback(const frame_callback &handle)
		{
			message_callback_handle_ = handle;
			return *this;
		}

		void do_parse(const void *data, uint32_t len)
		{
			if (data == nullptr || len == 0)
				throw std::runtime_error("data == nullptr");

			uint32_t offset = 0;
			uint32_t remain_len = len;
			try
			{
				do
				{
					if (parser_step_ == e_fixed_header && remain_len)
					{
						offset += parse_fixed_header((const char*)data + offset, remain_len);
						remain_len = len - offset;
					}
					if (parser_step_ == e_payload_len && remain_len)
					{
						offset += parse_payload_len((const char*)data + offset, remain_len);
						remain_len = len - offset;
					}
					if (parser_step_ == e_extened_payload_len && remain_len)
					{
						offset += parse_extened_payload_len((const char*)data + offset, remain_len);
						remain_len = len - offset;
					}
					if (parser_step_ == e_masking_key && remain_len)
					{
						offset += parse_masking_key((const char*)data + offset, remain_len);
						remain_len = len - offset;
					}
					if (parser_step_ == e_payload_data && remain_len)
					{
						offset += parse_payload((const char*)data + offset, remain_len);
						remain_len = len - offset;
					}

				} while (offset < len);
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
			
		}
	private:
		void move_reset(frame_parser &&parser)
		{
			if (&parser == this)
				return;
			parser_step_ = parser.parser_step_;
			payload_len_offset_ = parser.payload_len_offset_;
			masking_key_pos_ = parser.masking_key_pos_;
			payload_ = std::move(parser.payload_);
			frame_header_ = std::move(parser.frame_header_);
			message_callback_handle_ = std::move(parser.message_callback_handle_);
		}
		uint32_t parse_fixed_header(const char *data, uint32_t len)
		{
			const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);
			memset(&frame_header_, 0, sizeof(frame_header));
			payload_len_offset_ = 0;

			frame_header_.FIN_ = ptr[0] & 0xf0;
			frame_header_.RSV1_ = ptr[0] & 0x40;
			frame_header_.RSV2_ = ptr[0] & 0x20;
			frame_header_.RSV3_ = ptr[0] & 0x10;
			frame_header_.opcode_ = (frame_type)(ptr[0] & 0x0f);
			parser_step_ = e_payload_len;
			return 1;
		}

		uint32_t parse_payload_len(const char *data, uint32_t len)
		{
			const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);
			
			frame_header_.MASK_ = ptr[0] & 0x80;
			frame_header_.payload_len_ = ptr[0] & (0x7f);

			if (frame_header_.payload_len_ <= 125)
			{
				frame_header_.payload_realy_len_ = frame_header_.payload_len_;
				if (frame_header_.MASK_)
					parser_step_ = e_masking_key;
				else
					parser_step_ = e_payload_data;
			}
			else if(frame_header_.payload_len_ > 125)
			{
				parser_step_ = e_extened_payload_len;
			}

			if (frame_header_.payload_len_ == 0)
			{
				assert(message_callback_handle_);
				message_callback_handle_({}, frame_header_.opcode_, !!frame_header_.FIN_);
				reset();
			}

			return 1;
		}

		uint32_t parse_extened_payload_len(const char *data, uint32_t len)
		{
			uint32_t offset = 0;
			const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);

			if (frame_header_.payload_len_ == 126)
			{
				//Extended payload length is 16bit!
				uint32_t min_len = std::min<uint32_t>
					(2 - payload_len_offset_, len - offset);

				memcpy(&frame_header_.ext_payload_len_16_,
					ptr + offset, min_len);
				offset += min_len;
				payload_len_offset_ += min_len;
				if (payload_len_offset_ == 2)
					decode_extened_payload_len();
			}
			else if (frame_header_.payload_len_ == 127)
			{
				//Extended payload length is 64bit!
				auto min_len = std::min<uint32_t>(8 - payload_len_offset_, len - offset);
				memcpy(&frame_header_.ext_payload_len_64_, ptr + offset, (size_t)min_len);
				offset += min_len;
				payload_len_offset_ += min_len;
				if (payload_len_offset_ == 8)
					decode_extened_payload_len();
			}
			return offset;
		}

		void  decode_extened_payload_len()
		{
			if (frame_header_.payload_len_ == 126)
			{
				uint16_t tmp = frame_header_.ext_payload_len_16_;
				uint8_t *buffer_ = reinterpret_cast<uint8_t*>(&tmp);
				frame_header_.payload_realy_len_ =
					(((uint16_t)buffer_[0]) << 8) |((uint16_t)buffer_[1]);

			}
			else if (frame_header_.payload_len_ == 127)
			{
				uint64_t tmp = frame_header_.ext_payload_len_64_;
				uint8_t *buffer_ = reinterpret_cast<uint8_t*>(&tmp);
				frame_header_.payload_realy_len_ =
					(((uint64_t)buffer_[0]) << 56) |
					(((uint64_t)buffer_[1]) << 48) |
					(((uint64_t)buffer_[2]) << 40) |
					(((uint64_t)buffer_[3]) << 32) |
					(((uint64_t)buffer_[4]) << 24) |
					(((uint64_t)buffer_[5]) << 16) |
					(((uint64_t)buffer_[6]) << 8) |
					((uint64_t)buffer_[7]);
			}
			if(frame_header_.MASK_)
				parser_step_ = e_masking_key;
			else
			{
				parser_step_ = e_payload_data;
			}
		}

		uint32_t parse_masking_key(const char *data, uint32_t len)
		{
			const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);
			auto min = std::min<uint32_t>(4 - masking_key_pos_, len);
			if (parser_step_ == e_masking_key)
			{
				memcpy(&frame_header_.masking_key_, ptr, (size_t)min);
				masking_key_pos_ += min;
				if (masking_key_pos_ == 4)
					parser_step_ = e_payload_data;
			}
			return min;
		}

		uint32_t parse_payload(const char *data, uint32_t len)
		{
			if (payload_.empty())
				payload_.reserve((size_t)frame_header_.payload_realy_len_);

			auto remain = (uint32_t)frame_header_.payload_realy_len_ - (uint32_t)payload_.size();
			auto min_len = std::min<uint32_t>(remain, len);

			if (frame_header_.MASK_)
			{
				unsigned char *mask =
					(unsigned char *)&frame_header_.masking_key_;
				for (size_t i = 0; i < min_len; i++)
					payload_.push_back((char)((unsigned char)data[i] ^ mask[i % 4]));
			}
			else
				payload_.append(data, min_len);

			if (payload_.size() == frame_header_.payload_realy_len_)
			{
				assert(message_callback_handle_);
				message_callback_handle_(std::move(payload_), 
					frame_header_.opcode_,
					!!frame_header_.FIN_);
				reset();
			}
			return min_len;
		}
		void reset()
		{
			parser_step_ = e_fixed_header;
			masking_key_pos_ = 0;
			payload_len_offset_ = 0;
			payload_.clear();
			memset(&frame_header_, 0, sizeof(frame_header_));
		}
	private:
		enum parser_step
		{
			e_fixed_header,
			e_payload_len,
			e_extened_payload_len,
			e_masking_key,
			e_payload_data,
		};

		parser_step parser_step_;

		//extended payload length write pos
		uint32_t payload_len_offset_;

		//masking key buffer pos
		uint32_t masking_key_pos_;

		std::string payload_;

		frame_header frame_header_;

		frame_callback message_callback_handle_;
	};
}