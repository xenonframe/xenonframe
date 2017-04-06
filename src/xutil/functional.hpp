#pragma once
#include <cstring>
#include <time.h>
#include <ctype.h>
#include <string>
#include <string.h>
namespace xutil
{
	namespace functional
	{
		struct gmtime
		{
			struct tm operator()(time_t _time = time(nullptr))
			{
				struct tm gmt;
				std::memset(&gmt, 0, sizeof(gmt));
#if defined _MSC_VER
				gmtime_s(&gmt, &_time);
#else
				gmtime_r(&_time, &gmt);
#endif 
				return gmt;
			}
		};


		struct get_filename
		{
			std::string operator ()(const std::string &filepath)
			{
				auto pos = filepath.find_last_of('\\');
				if (pos == std::string::npos)
					pos = filepath.find_last_of('/');
				if (pos == std::string::npos)
					return filepath;
				++pos;
				return filepath.substr(pos, filepath.size() - pos);
			}
		};

		struct get_rfc1123
		{
			std::string operator()(time_t _time = time(nullptr))
			{
				auto gmt = gmtime()(_time);
				char buf[64] = {0};
				strftime(buf, 64 - 1, "%a, %d %b %Y %H:%M:%S GMT", &gmt);
				return buf;
			}
		};

		struct get_extension
		{
			std::string operator()(const std::string &file)
			{
				auto pos = file.find_last_of('.');
				if (pos == std::string::npos)
					return{};
				return file.substr(pos, file.size() - pos);
			}
		};
		
		struct strcasecmper
		{
			bool operator()(const char *left, const char *right)
			{
#ifdef _WIN32
				return _strcmpi(left, right) == 0;
#else
				return strcasecmp(left, right) == 0;
#endif
			}
		};
		inline std::string tolowerstr(const std::string &str)
		{
			std::string buffer;
			for (auto &itr : str)
				buffer.push_back((char)::tolower(itr));
			return buffer;
		}
		struct strncasecmper
		{
			bool operator()(const char *left, const char *right, std::size_t len)
			{
#ifdef _WIN32
				return _strnicmp(left, right, len) == 0;
#else
				return strncasecmp(left, right, len) == 0;
#endif
			}
		};

		struct strncmper
		{
			bool operator()(const char *left, const char *right, std::size_t len)
			{
				return strncmp(left, right, len) == 0;
			}
	};
	}
}
