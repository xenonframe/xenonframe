#pragma once
#include <string>
#include <list>
#include <functional>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
namespace xtest
{
#ifdef _WIN32
	static inline std::ostream& BLUE(std::ostream &s)

	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		return s;
	}
	static inline std::ostream& RED(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
		return s;

	}
	static inline std::ostream& GREEN(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		return s;
	}
	static inline std::ostream& YELLOW(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& WHITE(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		return s;
	}
	struct color 
	{
		color(WORD attribute) :m_color(attribute) {};
		WORD m_color;
	};
	template <class _Elem, class _Traits>
	std::basic_ostream<_Elem, _Traits>&
		operator<<(std::basic_ostream<_Elem, _Traits>& i, const color& c)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout, c.m_color);
		return i;
	}
#elif
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
#endif // _WIN32

	class xtest_excetion
	{
	public:
		xtest_excetion(const char *file,int line, const char *errstr)
		{
			error_str_ += "FILE: ";
			error_str_ += file;
			error_str_ += " LINE: ";
			error_str_ += std::to_string(line);
			error_str_ += "  ";
			error_str_ += errstr;
			
		}
		const char *str()
		{
			return error_str_.c_str();
		}
	private:
		std::string error_str_;
	};

	class xtest
	{
	public:
		typedef std::tuple<std::string, std::string, std::function<void(void)>> test_t;
		static xtest &get_inst()
		{
			static xtest inst;
			return inst;
		}
		xtest &add_test(const std::string &suite, const std::string &name,std::function<void()> test)
		{
			tests_.emplace_back(suite, name, test);
			return *this;
		}
		void run_test()
		{
			std::string suite;
			for (auto &itr : tests_)
			{
				if (suite != std::get<0>(itr))
				{
					if (suite.size())
						std::cout << "- - - - - - - - - - - - -"<< std::endl;
					std::cout << "suite: " << YELLOW <<
						std::get<0>(itr).c_str() << WHITE << std::endl;
				}
					
				suite = std::get<0>(itr);
				std::cout << "     |→ " << YELLOW <<std::get<1>(itr).c_str()<<WHITE;
				try
				{
					std::get<2>(itr)();
					std::cout << GREEN<<" √" << WHITE <<std::endl;
				}
				catch (xtest_excetion &e)
				{
					std::cout << RED <<" ㄨ "<< e.str() << WHITE << std::endl;
					continue;
				}
			}
		}
	private:
		void SetColor(unsigned short forecolor = 4, unsigned short backgroudcolor = 0)
		{
			HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE); //获取缓冲区句柄
			SetConsoleTextAttribute(hCon, forecolor | backgroudcolor); //设置文本及背景色
		}
		std::list<test_t> tests_;
	};
}

#define XTEST_SUITE(name) namespace name##_suite{ static const char *__suite_name  = #name; } namespace name##_suite

#define XUNIT_TEST(name) \
static inline void do_##name##_unit_test();\
class unit_test_##name\
{\
public:\
	unit_test_##name()\
	{\
		xtest::xtest::get_inst().add_test(__suite_name, #name, [&] { do_##name##_unit_test();});\
	}\
}_unit_test_##name;\
 void  do_##name##_unit_test()

#define  xassert(x) if (!(x)) throw xtest::xtest_excetion(__FILE__, __LINE__, "xassert( "#x" ) failed !");

#define XTEST_MAIN \
int main()\
{\
	xtest::xtest::get_inst().run_test();\
	getchar();\
	return 0;\
}
