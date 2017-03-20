#include "xactor/xactor.hpp"
#include "xutil/function_traits.hpp"
using namespace xactor;


struct mmm
{
	std::string hello_;
	RegistActorMsg(mmm, hello_);
};

class timer_test : public actor
{
public:
	timer_test() {};
	~timer_test() {};
private:
	void handle_mmm()
	{
		struct timer :public actor
		{
		public:
			timer(uint64_t &msgs)
				:msgs_(msgs)
			{

			}
			void init()
			{
				auto id = set_timer(1000, [this] {

					std::cout << msgs_ - last_msg_ << std::endl;
					last_msg_ = msgs_;
					return true;
				});
			}
			uint64_t &msgs_;
			uint64_t last_msg_ = 0;
		};


		regist([this](const address &from, mmm &&_mmm){
			msgs_++;
		});

		spawn<timer>(std::ref(msgs_));
	}
	void init_handle()
	{
		handle_mmm();
	}
	void init() 
	{
		init_handle();
	};
	int clicks_ = 1;
	uint64_t msgs_ = 0;
};

int main()
{
	xengine e;
	e.start();

	auto addr = e.spawn<timer_test>();
	auto addr1 = e.spawn<timer_test>();
	auto addr2 = e.spawn<timer_test>();
	auto addr3 = e.spawn<timer_test>();
	auto addr4 = e.spawn<timer_test>();
	auto addr5 = e.spawn<timer_test>();
	auto addr6 = e.spawn<timer_test>();

	auto addr7 = e.spawn<timer_test>();
	auto addr8 = e.spawn<timer_test>();
	auto addr9 = e.spawn<timer_test>();
	auto addr10 = e.spawn<timer_test>();


	for (int i = 0; i < 10000001; i++)
	{
		e.send(addr, mmm{ "from engine .1 " + std::to_string(i) });
		e.send(addr1, mmm{ "from engine .2 " + std::to_string(i) });
		e.send(addr2, mmm{ "from engine .3 " + std::to_string(i) });
		e.send(addr3, mmm{ "from engine .4 " + std::to_string(i) });
		e.send(addr4, mmm{ "from engine .5 " + std::to_string(i) });
		e.send(addr5, mmm{ "from engine .6 " + std::to_string(i) });
		e.send(addr6, mmm{ "from engine .7 " + std::to_string(i) });

		e.send(addr7, mmm{ "from engine .8 " + std::to_string(i) });
		e.send(addr8, mmm{ "from engine .9 " + std::to_string(i) });
		e.send(addr9, mmm{ "from engine .10 " + std::to_string(i) });
		e.send(addr10, mmm{ "from engine .11 " + std::to_string(i) });
	}

	getchar();
	e.stop();

	return 0;
}