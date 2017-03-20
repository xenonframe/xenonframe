#include "xutil/xworker_pool.hpp"
#include "G:\\fork\\akzi\\thread-pool-cpp-master\\include\\thread_pool.hpp"




uint64_t count_ =0;
uint64_t last_count_ = 0;

void print_()
{
	std::thread *thread = new std::thread([&] {
	
	do 
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::cout << count_ - last_count_ << std::endl;
		last_count_ = count_;
	} while (1);
	});
}

int main1()
{

	using namespace xutil;

	xworker_pool pool;

	for (size_t i = 0; i < 10000000; i++)
	{
		pool.add_job([&, i] {
			count_ = i;
		});
	}
	getchar();
	getchar();
	getchar();
	return 0;
}


int main2()
{
	tp::ThreadPoolOptions op;
	op.threads_count = 0;
	op.worker_queue_size = 1024 * 1024;
	tp::ThreadPool<> pool(op);

	for (size_t i = 0; i < 1000000000; i++)
	{
		pool.post([&, i] {
			count_ = i;
		});
	}
	getchar();
	return 0;
}

int main()
{
	print_();
	if (getchar() == ' ')
	{
		main1();
	}else
		main2();

	return 0;
}