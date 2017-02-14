#include "xtest/xtest.hpp"
#include "xutil/functional.hpp"
#include "xhttper/http_parser.hpp"
#include "xhttper/http_builder.hpp"
#include <thread>
#include <iostream>
XTEST_MAIN;

using namespace xutil::functional;
XTEST_SUITE(xhttper)
{
#if 1
	XUNIT_TEST(parse)
	{
		const char *buf =
			"GET /eclick.baidu.com/a.js?tu=u2310667&jk=3f4ff730444a9cb7&word=http%3A%2F%2Fe"
			".firefoxchina.cn%2F%3Fcachebust%3D20160321&if=3&aw=250&ah=108&pt=96500&it=96500&vt="
			"96500&csp=1920,1040&bcl=250,120&pof=250,120&top=0&left=0&total=1&rdm=1479249557254 HTTP/1.1\r\n"
			"Host: eclick.baidu.com\r\n"
			"User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; rv:50.0) Gecko/20100101 Firefox/50.0\r\n"
			"Accept: */*\r\n"
			"Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3\r\n"
			"Accept-Encoding: gzip, deflate\r\n"
			"Referer: http://fragment.firefoxchina.cn/html/main_baidu_cloud_250x108.html\r\n"
			"Cookie: BAIDUID=9DF780D9B96413B1421F2758E92D4DEB:FG=1\r\n"
			"Connection: keep-alive\r\n\r\nhello world";

		xhttper::http_parser per;

		auto size = strlen(buf);
		for (int i =0 ; i < size; i++)
		{
			per.append(buf + i, 1);
			if (per.parse_req())
			{
				per.append(buf + i + 1, size - i - 1);
				break;
			}
		}

		xassert(per.get_method() == "GET");
		xassert(per.url() == "/eclick.baidu.com/a.js?tu=u2310667&jk=3f4ff730444a9cb7&w"
			"ord=http%3A%2F%2Fe.firefoxchina.cn%2F%3Fcachebust%3D20160321&if=3&aw"
			"=250&ah=108&pt=96500&it=96500&vt=96500&csp=1920,1040&bcl=250,120&pof"
			"=250,120&top=0&left=0&total=1&rdm=1479249557254");
		xassert(per.get_version() == "HTTP/1.1");

		xassert(per.get_header<strncasecmper>("Host") == "eclick.baidu.com");
		xassert(per.get_header<strncasecmper>("User-Agent") == "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:50.0) Gecko/20100101 Firefox/50.0");
		xassert(per.get_header<strncasecmper>("Accept") == "*/*");
		xassert(per.get_header<strncasecmper>("Accept-Language") == "zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
		xassert(per.get_header<strncasecmper>("Accept-Encoding") == "gzip, deflate");
		xassert(per.get_header<strncasecmper>("Referer") == "http://fragment.firefoxchina.cn/html/main_baidu_cloud_250x108.html");
		xassert(per.get_header<strncasecmper>("Cookie") == "BAIDUID=9DF780D9B96413B1421F2758E92D4DEB:FG=1");
		xassert(per.get_header<strncasecmper>("Connection") == "keep-alive");
		per.reset();
		xassert(per.get_string(strlen("hello world"))== "hello world");
	}
	XUNIT_TEST(Benchmark)
	{
		const char *buf = "GET /cookies HTTP/1.1\r\nHost: 127.0.0.1:8090\r\nConnection:"
			" keep-alive\r\nCache-Control:"
			" max-age=0\r\nAccept: text/html"
			",application/xhtml+xml,application/xml;q=0.9,*/*;"
			"q=0.8\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) "
			"AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.56 Safari/537.17\r\n"
			"Accept-Encoding: gzip,deflate,sdch\r\n"
			"Accept-Language: en-US,en;q=0.8\r\n"
			"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n"
			"Cookie: name=wookie\r\n\r\n";

		auto buflen =  strlen(buf);
		int64_t count = 0;
		bool stop = false;
		std::thread counter([&] {
			int seconds = 5;
			auto last_count = count;
			auto count_ = count;
			auto len = strlen(buf);
			std::cout << std::endl;;
			do
			{
				count_ = count;
				std::cout << (count_ - last_count)*buflen / 1024 / 1024 << \
					" MB " << (count_ - last_count) << std::endl;
				last_count = count_;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			} while (seconds -- > 0);
			stop = true;
		});
		xhttper::http_parser per;
		counter.detach();
		do
		{
			per.append(buf, strlen(buf));
			xassert(per.parse_req());
			per.reset();
			count++;
		} while (stop == false);
	}
#endif
	XUNIT_TEST(parse_rsp)
	{
		const char *buf = 
			"HTTP/1.1 200 OK\r\n"
			"Server: Microsoft-IIS/5.0\r\n"
			"Date: Thu,08 Mar 200707:17:51 GMT\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 23330\r\n"
			"Content-Type: text/html\r\n"
			"Expries: Thu,08 Mar 2007 07:16:51 GMT\r\n"
			"Set-Cookie: ASPSESSIONIDQAQBQQQB=BEJCDGKADEDJKLKKAJEOIMMH; path=/\r\n"
			"Cache-control: private\r\n\r\n";

		xhttper::http_parser per;
		int length = (int)strlen(buf);
		for (size_t i = 0; i < length; i++)
		{
			per.append(buf + i, 1);
			if (per.parse_rsp())
				break;
		}
		xassert(per.get_version() == "HTTP/1.1");
		xassert(per.get_status() == "200");
		xassert(per.get_status_str() == "OK");
		xassert(per.get_header<strncasecmper>("Server") == "Microsoft-IIS/5.0");
		xassert(per.get_header<strncasecmper>("Date") == "Thu,08 Mar 200707:17:51 GMT");
		xassert(per.get_header<strncasecmper>("Connection") == "Keep-Alive");
		xassert(per.get_header<strncasecmper>("Content-Length") == "23330");
		xassert(per.get_header<strncasecmper>("Content-Type") == "text/html");
		xassert(per.get_header<strncasecmper>("Expries") == "Thu,08 Mar 2007 07:16:51 GMT");
		xassert(per.get_header<strncasecmper>("Set-Cookie") == "ASPSESSIONIDQAQBQQQB=BEJCDGKADEDJKLKKAJEOIMMH; path=/");
		xassert(per.get_header<strncasecmper>("Cache-control") == "private");
	}
}
#include <sstream>
XTEST_SUITE(builder)
{
	const char *buf =
		"HTTP/1.1 200 OK\r\n"
		"Server: Microsoft-IIS/5.0\r\n"
		"Date: Thu,08 Mar 2007 07:17:51 GMT\r\n"
		"Connection: Keep-Alive\r\n"
		"Content-Length: 23330\r\n"
		"Content-Type: text/html\r\n"
		"Expries: Thu,08 Mar 2007 07:16:51 GMT\r\n"
		"Set-Cookie: ASPSESSIONIDQAQBQQQB=BEJCDGKADEDJKLKKAJEOIMMH; path=/\r\n"
		"Cache-control: private\r\n\r\n";

	XUNIT_TEST(build)
	{
		xhttper::http_builder ber;
		ber.append_entry("Server", "Microsoft-IIS/5.0");
		ber.append_entry("Date", "Thu,08 Mar 2007 07:17:51 GMT");
		ber.append_entry("Connection", "Keep-Alive");
		ber.append_entry("Content-Length", "23330");
		ber.append_entry("Content-Type", "text/html");
		ber.append_entry("Expries", "Thu,08 Mar 2007 07:16:51 GMT");
		ber.append_entry("Set-Cookie","ASPSESSIONIDQAQBQQQB=BEJCDGKADEDJKLKKAJEOIMMH; path=/");
		ber.append_entry("Cache-control", "private");

		auto data = ber.build_resp();
	}
	XUNIT_TEST(encode_chunked)
	{
		xassert(xhttper::http_builder().encode_chunked(std::string('*', 42))\
			== "2a\r\n" + std::string('*', 42) + "\r\n")
	}
}