#pragma once
#include <stdint.h>
namespace xwebsocket
{
	enum frame_type
	{
		e_continuation = 0x00,
		e_text = 0x01,
		e_binary = 0x02,
		e_connection_close = 0x08,
		e_ping = 0x09,
		e_pong = 0xa
	};

	struct frame_header
	{
		uint8_t FIN_ = 0;
		uint8_t RSV1_ = 0;
		uint8_t RSV2_ = 0;
		uint8_t RSV3_ = 0;
		frame_type opcode_ = e_continuation;
		uint8_t MASK_ = 0;
		uint8_t payload_len_ = 0;
		uint16_t ext_payload_len_16_ = 0; //extended payload length 16
		uint64_t ext_payload_len_64_ = 0; //extended payload length 64
		uint64_t payload_realy_len_ = 0;
		uint32_t masking_key_ = 0;
	};
}