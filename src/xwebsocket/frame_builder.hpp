#pragma once
namespace xwebsocket
{
	class frame_maker
	{
	public:
		frame_maker()
		{
			
		}
		frame_maker &reset_masking_key()
		{
			frame_header_.MASK_ = 0x00;
			return *this;
		}
		frame_maker &set_masking_key(uint32_t masking_key)
		{
			frame_header_.MASK_ = 0x80;
			frame_header_.masking_key_ = masking_key;
			return *this;
		}
		frame_maker &set_fin(bool val)
		{
			frame_header_.FIN_ = val ? 0x80 : 0;
			return *this;
		}
		frame_maker &set_frame_type(frame_type type)
		{
			frame_header_.opcode_ = type;
			return *this;
		}
		std::string make_frame(const char* data, size_t len)
		{
			std::string buffer;
			int externded_payload_len = (len <= 125 ? 0 : (len <= 65535 ? 2 : 8));
			int mask_key_len = ((len && frame_header_.MASK_) ? 4 : 0);

			size_t frame_size =
				2 +
				externded_payload_len
				 +
				mask_key_len
				+
				len;

			buffer.resize(frame_size);
			uint8_t *ptr = (uint8_t*)buffer.data();
			uint64_t offset = 0;

			ptr[0] |= frame_header_.FIN_;
			ptr[0] |= frame_header_.opcode_;

			if (len)
				ptr[1] |= frame_header_.MASK_;

			offset += 1;
			if (len <= 125)
			{
				ptr[offset++] |= (unsigned char)len;
			}
			else if (len <= 65535)
			{
				ptr[offset++] |= 126;
				ptr[offset++] = (unsigned char)(len >> 8) & 0xFF;
				ptr[offset++] = len & 0xFF;
			}
			else
			{
				ptr[offset++] |= 127;
				ptr[offset++] = (unsigned char)(((uint64_t)len >> 56) & 0xff);
				ptr[offset++] = (unsigned char)(((uint64_t)len >> 48) & 0xff);
				ptr[offset++] = (unsigned char)(((uint64_t)len >> 40) & 0xff);
				ptr[offset++] = (unsigned char)(((uint64_t)len >> 32) & 0xff);
				ptr[offset++] = (unsigned char)(((uint64_t)len >> 24) & 0xff);
				ptr[offset++] = (unsigned char)(((uint64_t)len >> 16) & 0xff);
				ptr[offset++] = (unsigned char)(((uint64_t)len >> 8) & 0xff);
				ptr[offset++] = (unsigned char)((uint64_t)len & 0xff);
			}
			
			if (!len)
				return buffer;

			if (frame_header_.MASK_)
			{
				int mask_key = frame_header_.masking_key_;
				ptr[offset++] = (unsigned char)((mask_key >> 24) & 0xff);
				ptr[offset++] = (unsigned char)((mask_key >> 16) & 0xff);
				ptr[offset++] = (unsigned char)((mask_key >> 8) & 0xff);
				ptr[offset++] = (unsigned char)((mask_key) & 0xff);

				unsigned char *mask = ptr + offset - 4;
				for (uint32_t i = 0; i < len; i++)
				{
					ptr[offset++] = data[i] ^ mask[i % 4];
				}
			}
			else
			{
				memcpy((void*)(ptr + offset), data, (size_t)len);
				offset += len;
			}
			assert(offset == frame_size);
			return buffer;
		}
	private:
		frame_header frame_header_;
	};
}