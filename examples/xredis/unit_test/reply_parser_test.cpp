#include "../include/xredis.hpp"
#if 0
XTEST_SUITE(reply_parser)
{
	XUNIT_TEST(parse)
	{
		const std::string buffer =
			"*2\r\n"
			"*5\r\n:4000\r\n:16383\r\n"
			"*3\r\n$13\r\n192.168.3.224\r\n:7000\r\n$40\r\nb9ab1cef8c926f9771fd4efa16d047391a179e4d\r\n"
			"*3\r\n$13\r\n192.168.3.224\r\n:7002\r\n$40\r\n232eb1f44c2860a2b17b9d1cc2d3cd5854a728aa\r\n"
			"*3\r\n$13\r\n192.168.3.224\r\n:7001\r\n$40\r\nc6d8894dc6b281488185a4734578eda2cf80442b\r\n"
			"*5\r\n:0\r\n:3999\r\n"
			"*3\r\n$13\r\n192.168.3.224\r\n:700";
		const std::string buffer2 = 
					"3\r\n$40\r\n138eb855c041bbe2b8bb72e6f38c3a2d17697884\r\n"
					"*3\r\n$13\r\n192.168.3.224\r\n:7005\r\n$40\r\n308d41acf591ec905c577e997942fcded12882be\r\n"
					"*3\r\n$13\r\n192.168.3.224\r\n:7004\r\n$40\r\n0fcfcfdbd9228b3d90f3f2dc461447ae037091ba\r\n";

		xredis::detail::reply_parser parser;

		xredis::cluster_slots_callback cb = 
			[](std::string && status, xredis::cluster_slots &&cluster_slots) 
		{ 

		};

		parser.regist_callback(cb);

		parser.parse(buffer.data(), buffer.size());
		parser.parse(buffer2.data(), buffer2.size());
	}
}
XTEST_SUITE(xredis)
{
	XUNIT_TEST(parse_cluster_info)
	{
		const char *info = 
			"232eb1f44c2860a2b17b9d1cc2d3cd5854a728aa 192.168.3.224:7002 slave b9ab1cef8c926f9771fd4efa16d047391a179e4d 0 1478765683266 0 connected\n"
			"b9ab1cef8c926f9771fd4efa16d047391a179e4d 192.168.3.224:7000 myself, master - 0 0 0 connected 4000-16383\n"
			"0fcfcfdbd9228b3d90f3f2dc461447ae037091ba 192.168.3.224:7004 slave 138eb855c041bbe2b8bb72e6f38c3a2d17697884 0 1478765685269 1 connected\n"
			"138eb855c041bbe2b8bb72e6f38c3a2d17697884 192.168.3.224:7003 master - 0 1478765684768 1 connected 0-3999\n"
			"308d41acf591ec905c577e997942fcded12882be 192.168.3.224:7005 slave 138eb855c041bbe2b8bb72e6f38c3a2d17697884 0 1478765683768 1 connected\n"
			"c6d8894dc6b281488185a4734578eda2cf80442b 192.168.3.224:7001 slave b9ab1cef8c926f9771fd4efa16d047391a179e4d 0 1478765684268 0 connected\n";

		auto masters = xredis::master_info_parser()(info);
		xassert(masters.size() == 2);
	}
}
XTEST_SUITE(map)
{
	XUNIT_TEST(lower_bound)
	{
		std::map<char, int> mymap;
		std::map<char, int>::iterator itlow, itup;

		mymap[20] = 20;
		mymap[40] = 40;
		mymap[60] = 60;
		mymap[80] = 80;
		mymap[100] = 100;

		itlow = mymap.lower_bound(10); 
		xassert(itlow->second == 20);
		itup = mymap.lower_bound(30);
		xassert(itup->second == 40);
	}
}
#endif 