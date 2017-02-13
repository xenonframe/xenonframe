#if 1
#include "../include/raft/raft.hpp"
#include "../../xtest/include/xtest.hpp"

XTEST_SUITE(metadata)
{
	XUNIT_TEST(do_metadata_set)
	{
		using namespace::xraft::detail;
		metadata<lock_free> db;

		db.init("data/");
		
		for (size_t i = 0; i < 10000; i++)
		{
			db.set(std::to_string(i), std::to_string(i));
		}
	}

	XUNIT_TEST(do_metadata_get)
	{
		using namespace::xraft::detail;
		metadata<lock_free> db;

		db.init("data/");

		for (size_t i = 0; i < 10000; i++)
		{
			std::string value;
			xassert(db.get(std::to_string(i), value));
			xassert(std::to_string(i) == value);
		}
	}
	XUNIT_TEST(do_metadata_del)
	{
		using namespace::xraft::detail;
		metadata<lock_free> db;

		db.init("data/");

		for (size_t i = 0; i < 10000; i++)
		{
			std::string value;
			xassert(db.del(std::to_string(i)));
		}
	}
#if 0
	XUNIT_TEST(do_metadata_batchmark)
	{
		using namespace::xraft::detail;
		metadata<lock_free> db;

		xassert(db.init("data/"));
		int64_t count_ = 0;
		bool is_stop = false;
		std::cout << std::endl;
		std::thread counter([&] {
			auto  last = count_;
			do
			{
				std::cout << "writes:" << count_ - last << std::endl;;
				std::cout << "count:" << count_ << std::endl;;
				last = count_;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			} while (is_stop == false);
		});
		counter.detach();
		for (size_t i = 0; i < 1000000; i++)
		{
			count_++;
			xassert(db.set(std::to_string(i), std::to_string(i)));
		}
		is_stop = true;
		std::cout << "ok" << std::endl;
	}
#endif
	
}
#endif // 0
