#pragma once
namespace xnet
{
	namespace detail
	{
		struct socket_exception :public std::exception
		{
		public:
			socket_exception(const char *file, 
				const int line, int error_code)
				:error_code_(error_code)
			{
				error_str_ += "FILE: ";
				error_str_ += file;
				error_str_ += " LINE: ";
				error_str_ += std::to_string(line);
				error_str_ += " error_code:";
				error_str_ += std::to_string(error_code_);
				error_str_ += " error_str:";
				init_error_msg();
				std::cout << error_str_.c_str() << std::endl;;
			}
		private:
			void init_error_msg()
			{
#ifdef _MSC_VER
				static char errmsg[512];

				if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,
				   error_code_,0,errmsg,511,NULL))
					error_str_ = "FormatMessage failed";
				else
					error_str_ += errmsg;
#elif defined(_LINUX_)
				const char *errmsg = strerror(error_code_);
				if(errmsg)
					error_str_ += errmsg;
				else
					error_str_ = "strerror error";
#endif
			}

			virtual char const* what() const throw()
			{
				return error_str_.c_str();
			}

			int error_code_ = 0;
			std::string error_str_;
		};
#ifdef _MSC_VER
#define xnet_assert(x) \
		if(!(x)) \
			throw detail::socket_exception\
			(__FILE__, __LINE__, WSAGetLastError());
#elif defined(_LINUX_)
#define xnet_assert(x) \
		if(!(x)) \
			throw detail::socket_exception\
			(__FILE__, __LINE__, errno);
#endif
	}
}
