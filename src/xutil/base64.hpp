#pragma once
#include <string>

namespace xutil
{
namespace base64
{
	static constexpr unsigned char to_b64_tab[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	static constexpr unsigned char un_b64_tab[] =
	{
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255, 255, 63,
		52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 255, 255, 255,
		255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
		15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255, 255, 255, 255, 255,
		255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
		41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	};

	static inline std::string encode(const std::string &data)
	{
		const uint8_t *ptr = (const uint8_t*)data.c_str();
		int len = (int)data.size();

		std::string buffer;
		buffer.reserve(4 * (len + 2) / 3);

		while (len-- > 0) 
		{
			register int x, y;
			x = *ptr++;
			buffer.push_back((char)to_b64_tab[(x >> 2) & 63]);

			if (len-- <= 0)
			{
				buffer.push_back((char)to_b64_tab[(x << 4) & 63]);
				buffer.push_back((char)'=');
				buffer.push_back((char)'=');
				break;
			}

			y = *ptr++;
			buffer.push_back((char)(to_b64_tab[((x << 4) | ((y >> 4) & 15)) & 63]));

			if (len-- <= 0)
			{
				buffer.push_back((char)to_b64_tab[(y << 2) & 63]);
				buffer.push_back((char)'=');
				break;
			}

			x = *ptr++;
			buffer.push_back((char)(to_b64_tab[((y << 2) | ((x >> 6) & 3)) & 63]));
			buffer.push_back((char)(to_b64_tab[x & 63]));
		}
		return buffer;
	}

	static inline bool decode(const std::string &in, std::string &out)
	{
		out.reserve(3 * (in.size()) / 4);

		const uint8_t *code = (const uint8_t *)in.c_str();
		register int x, y;

		while ((x = (*code++)) != 0)
		{
			if (x > 127 || (x = un_b64_tab[x]) == 255)
				return false;
			if ((y = (*code++)) == 0 || (y = un_b64_tab[y]) == 255)
				return false;

			out.push_back((char)((x << 2) | (y >> 4)));

			if ((x = (*code++)) == '=')
			{
				if (*code++ != '=' || *code != 0)
					return false;
			}
			else
			{
				if (x > 127 || (x = un_b64_tab[x]) == 255)
					return false;

				out.push_back((char)((y << 4) | (x >> 2)));

				if ((y = (*code++)) == '=')
				{
					if (*code != 0)
						return false;
				}
				else
				{
					if (y > 127 || (y = un_b64_tab[y]) == 255)
						return false;
					out.push_back((char)((x << 6) | y));
				}
			}
		}
		return true;
	}
}
}

