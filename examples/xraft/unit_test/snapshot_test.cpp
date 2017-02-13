#if 0

#include "../include/raft/raft.hpp"
#include "../../xtest/include/xtest.hpp"
XTEST_SUITE(snapshot)
{
	XUNIT_TEST(checker)
	{

		xraft::snapshot_reader reader;
		xraft::snapshot_head head;
		xassert(reader.open("2631.ss"));
		xassert(reader.read_sanpshot_head(head));


		xraft::snapshot_writer writer;
		std::string filepath = "2631.sss";
		if (!writer.open(filepath))
			throw std::runtime_error("open " + filepath + "error");
		head.last_included_index_ = 2631;
		head.last_included_term_ = 2;
		xassert(writer.write_sanpshot_head(head));

		writer.close();
		xassert(reader.open("2631.sss"));
		xassert(reader.read_sanpshot_head(head));
	}
}

#endif