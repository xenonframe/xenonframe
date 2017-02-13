#pragma once

#ifndef DEFINE_ERROR
#define DEFINE_ERROR(ClassName)\
class ClassName\
	:public std::logic_error\
{\
public:\
	explicit ClassName(const std::string & msg)\
		:std::logic_error(msg)\
	{\
\
	}\
	explicit ClassName(const char *msg)\
		:std::logic_error(msg)\
	{\
\
	}\
};
#endif // !DEFINE_ERROR

#ifndef COUT_ERROR
#define COUT_ERROR(e) std::cout << "FUNC:"<<__FUNCTION__ << \
" LINE:"<<__LINE__<<" ERROR:"<<e.what() <<std::endl;

#endif // !COUT_ERROR



DEFINE_ERROR(packet_error);
DEFINE_ERROR(lost_connection);
DEFINE_ERROR(Upgrade);

