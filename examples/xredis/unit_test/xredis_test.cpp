#include "../include/xredis.hpp"

xtest_run;

XTEST_SUITE(xredis)
{
#if 0
	XUNIT_TEST(set_addr)
	{
		xnet::proactor proactor;

		xredis::redis redis(proactor);
		redis.set_addr("192.168.3.224", 7000);
		proactor.set_timer(500000, [&] {
			proactor.stop();
			return false;
		});
		proactor.run();
		
	}
#endif
}