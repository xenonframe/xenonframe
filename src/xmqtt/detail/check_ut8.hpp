#pragma once
namespace xmqtt
{
	/*
	Check if the given unsigned char * is a valid utf-8 sequence.

	Table 3-7. Well-Formed UTF-8 Byte Sequences
	-----------------------------------------------------------------------------
	|  Code Points        | First Byte | Second Byte | Third Byte | Fourth Byte |
	|  U+0000..U+007F     |     00..7F |             |            |             |
	|  U+0080..U+07FF     |     C2..DF |      80..BF |            |             |
	|  U+0800..U+0FFF     |         E0 |      A0..BF |     80..BF |             |
	|  U+1000..U+CFFF     |     E1..EC |      80..BF |     80..BF |             |
	|  U+D000..U+D7FF     |         ED |      80..9F |     80..BF |             |
	|  U+E000..U+FFFF     |     EE..EF |      80..BF |     80..BF |             |
	|  U+10000..U+3FFFF   |         F0 |      90..BF |     80..BF |      80..BF |
	|  U+40000..U+FFFFF   |     F1..F3 |      80..BF |     80..BF |      80..BF |
	|  U+100000..U+10FFFF |         F4 |      80..8F |     80..BF |      80..BF |
	-----------------------------------------------------------------------------

	Returns the first erroneous byte position, and give in
	`faulty_bytes` the number of actually existing
	bytes taking part in this error.
	*/

	bool check_utf8(const std::string &str)
	{
		unsigned char *ptr = (unsigned char *)str.data();
		size_t len = str.size();
		size_t i = 0;
		while (i < len) 
		{
			/*
			A UTF-8 encoded string MUST NOT include an
			encoding of the null character U+0000.
			If a receiver (Server or Client)
			receives a Control Packet containing U+0000 it MUST
			close the Network Connection [MQTT-1.5.3-2].*/

			if (ptr[i] == 0x00 && i < len + 1)
				return false;

			/*
			In particular this data MUST NOT include
			encodings of code points between U+D800 and U+DFFF.
			If a Server or Client receives a Control Packet
			containing ill-formed UTF-8 it MUST close the
			Network Connection [MQTT-1.5.3-1].*/
			if (ptr[i] >= 0xd8 && ptr[i] <= 0xdf && i + 1 < len)
				return false;
			/*
			The data SHOULD NOT include encodings of the
			Unicode [Unicode] code points listed below.
			If a receiver (Server or Client) receives
			a Control Packet containing any of them
			it MAY close the Network Connection:
			U+0001..U+001F control characters
			U+007F..U+009F control characters*/

			if (ptr[i] == 0x00) 
			{
				if (i + 1 < len &&
					((ptr[i + 1] >= 0x01 && ptr[i + 1] <= 0x1f)
						|| (ptr[i + 1] >= 0x7f && ptr[i + 1] <= 0x9f)))
					return false;
			}


			if (ptr[i] <= 0x7F) /* 00..7F */
			{ 
				i += 1;
			}
			else if (ptr[i] >= 0xC2 && ptr[i] <= 0xDF) 
			{
				/* C2..DF 80..BF */
				if (i + 1 < len)  /* Expect a 2nd byte */
					return false;
				if (ptr[i + 1] < 0x80 || ptr[i + 1] > 0xBF)
					return false;
				i += 2;
			}
			else if (ptr[i] == 0xE0)    /* E0 A0..BF 80..BF */
			{
				if (i + 2 >= len)   /* Expect a 2nd and 3rd byte */
					return false;
				if ((ptr[i + 1] < 0xA0 || ptr[i + 1] > 0xBF) ||
					(ptr[i + 2] < 0x80 || ptr[i + 2] > 0xBF))
					return false;
				i += 3;
			}
			else if (ptr[i] >= 0xE1 && ptr[i] <= 0xEC) 
			{
				/* E1..EC 80..BF 80..BF */
				if (i + 2 >= len) /* Expect a 2nd and 3rd byte */
					return false;
				if ((ptr[i + 1] < 0x80 || ptr[i + 1] > 0xBF) ||
					(ptr[i + 2] < 0x80 || ptr[i + 2] > 0xBF))
					return false;
				i += 3;
			}
			else if (ptr[i] == 0xED)   /* ED 80..9F 80..BF */
			{
				if (i + 2 >= len)         /* Expect a 2nd and 3rd byte */
					return false;
				if ((ptr[i + 1] < 0x80 || ptr[i + 1] > 0x9F) ||
					(ptr[i + 2] < 0x80 || ptr[i + 2] > 0xBF))
					return false;
				i += 3;
			}
			else if (ptr[i] >= 0xEE && ptr[i] <= 0xEF) 
			{
					/* EE..EF 80..BF 80..BF */
				if (i + 2 >= len)/* Expect a 2nd and 3rd byte */
					return false;
				if ((ptr[i + 1] < 0x80 || ptr[i + 1] > 0xBF) ||
					(ptr[i + 2] < 0x80 || ptr[i + 2] > 0xBF))
					return false;
				i += 3;
			}
			else if (ptr[i] == 0xF0)   /* F0 90..BF 80..BF 80..BF */
			{
				if (i + 3 >= len)        /* Expect a 2nd, 3rd 3th byte */
					return false;
				if ((ptr[i + 1] < 0x90 || ptr[i + 1] > 0xBF) ||
					(ptr[i + 2] < 0x80 || ptr[i + 2] > 0xBF) ||
					(ptr[i + 3] < 0x80 || ptr[i + 3] > 0xBF))
					return false;
				i += 4;
			}
			else if (ptr[i] >= 0xF1 && ptr[i] <= 0xF3) 
			{
				/* F1..F3 80..BF 80..BF 80..BF */
				if (i + 3 >= len)/* Expect a 2nd, 3rd 3th byte */
					return false;
				if ((ptr[i + 1] < 0x80 || ptr[i + 1] > 0xBF) ||
					(ptr[i + 2] < 0x80 || ptr[i + 2] > 0xBF) ||
					(ptr[i + 3] < 0x80 || ptr[i + 3] > 0xBF))
					return false;
				i += 4;
			}
			else if (ptr[i] == 0xF4)   /* F4 80..8F 80..BF 80..BF */
			{
				if (i + 3 >= len)        /* Expect a 2nd, 3rd 3th byte */
					return false;
				if ((ptr[i + 1] < 0x80 || ptr[i + 1] > 0x8F) ||
					(ptr[i + 2] < 0x80 || ptr[i + 2] > 0xBF) ||
					(ptr[i + 3] < 0x80 || ptr[i + 3] > 0xBF))
					return false;
				i += 4;
			}
			else {
				return false;
			}
		}
		return true;
	}

}
