#pragma once
namespace xraft
{
	namespace endec
	{
		inline void put_bool(unsigned char *&buffer_, bool value)
		{
			*buffer_ = value ? 1:0;
			buffer_ += sizeof(char);
		}
		
		inline bool get_bool(unsigned char *&buffer_)
		{
			uint8_t value = buffer_[0];
			buffer_ += sizeof(value);
			return value > 0;
		}

		inline void put_uint8(unsigned char *&buffer_, uint8_t value)
		{
			*buffer_ = value;
			buffer_ += sizeof(value);
		}

		inline uint8_t get_uint8(unsigned char *&buffer_)
		{
			uint8_t value = buffer_[0];
			buffer_ += sizeof(value);
			return value;
		}

		inline void put_uint16(unsigned char *buffer_, uint16_t value)
		{
			buffer_[0] = (unsigned char)(((value) >> 8) & 0xff);
			buffer_[1] = (unsigned char)(value & 0xff);
			buffer_ += sizeof(value);
		}

		inline uint16_t get_uint16(unsigned char *&buffer_)
		{
			uint16_t value = (((uint16_t)buffer_[0]) << 8)|((uint16_t)buffer_[1]);
			buffer_ += sizeof(value);
			return value;
		}

		inline void put_uint32(unsigned char *&buffer_, uint32_t value)
		{
			buffer_[0] = (unsigned char)(((value) >> 24) & 0xff);
			buffer_[1] = (unsigned char)(((value) >> 16) & 0xff);
			buffer_[2] = (unsigned char)(((value) >> 8) & 0xff);
			buffer_[3] = (unsigned char)(value & 0xff);
			buffer_ += sizeof(value);
		}

		inline uint32_t get_uint32(unsigned char *&buffer_)
		{
			uint32_t value =
				(((uint32_t)buffer_[0]) << 24) |
				(((uint32_t)buffer_[1]) << 16) |
				(((uint32_t)buffer_[2]) << 8) |
				((uint32_t)buffer_[3]);
			buffer_ += sizeof(value);
			return value;
		}

		inline void put_uint64(unsigned char *&buffer_, uint64_t value)
		{
			buffer_[0] = (unsigned char)(((value) >> 56) & 0xff);
			buffer_[1] = (unsigned char)(((value) >> 48) & 0xff);
			buffer_[2] = (unsigned char)(((value) >> 40) & 0xff);
			buffer_[3] = (unsigned char)(((value) >> 32) & 0xff);
			buffer_[4] = (unsigned char)(((value) >> 24) & 0xff);
			buffer_[5] = (unsigned char)(((value) >> 16) & 0xff);
			buffer_[6] = (unsigned char)(((value) >> 8) & 0xff);
			buffer_[7] = (unsigned char)(value & 0xff);
			buffer_ += sizeof(value);
		}

		inline uint64_t get_uint64(unsigned char *&buffer_)
		{
			uint64_t value = 
				((((uint64_t)buffer_[0]) << 56) |
				(((uint64_t)buffer_[1]) << 48) |
				(((uint64_t)buffer_[2]) << 40) |
				(((uint64_t)buffer_[3]) << 32) |
				(((uint64_t)buffer_[4]) << 24) |
				(((uint64_t)buffer_[5]) << 16) |
				(((uint64_t)buffer_[6]) << 8) |
				((uint64_t)buffer_[7]));
			buffer_ += sizeof(value);
			return value;
		}
		inline void put_string(unsigned char *&buffer_, const std::string &str)
		{
			put_uint32(buffer_, (uint32_t)str.size());
			memcpy(buffer_, str.data(), str.size());
			buffer_ += str.size();
		}
		inline std::string get_string(unsigned char *&buffer_)
		{
			auto len = get_uint32(buffer_);
			std::string result((char*)buffer_, len);
			buffer_ += len;
			return std::move(result);
		}
		template <typename T>
		inline typename std::enable_if<std::is_arithmetic<T>::value, std::size_t>::type 
			get_sizeof(T)
		{
			return sizeof(T);
		}

		template <typename T>
		inline typename std::enable_if<std::is_same<std::string,T>::value, std::size_t>::type
			get_sizeof(const T &t)
		{
			return sizeof(uint32_t) + t.size();
		}
	}
}