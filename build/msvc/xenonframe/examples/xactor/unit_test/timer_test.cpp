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
		struct spawn_ :public actor
		{
			void init()
			{
				std::cout << "spawn_ init" << std::endl;
			}
		};
		regist([this](const address &from, mmm &&_mmm){
			
			if(msgs_++ % 100000== 0)
				std::cout << _mmm.hello_ << std::endl;
		});
		//spawn<spawn_>();
	}
	void init_handle()
	{
		handle_mmm();
	}
	void init() 
	{
		init_handle();
		auto id = set_timer(1, [this] {

			std::cout << "click." <<std::endl;
			if (clicks_ < 10)
			{
				clicks_++;
				return true;
			}
			return false;
		});
	};
	int clicks_ = 1;
	int msgs_ = 0;
};

int main()
{
	xengine e;
	e.start();

	auto addr = e.spawn<timer_test>();
	auto addr1 = e.spawn<timer_test>();

	for (int i = 0; i < 10000001; i++)
	{
		e.send(addr, mmm{ "from engine .1 " + std::to_string(i) });
		e.send(addr1, mmm{ "from engine .2 " + std::to_string(i) });
	}

	getchar();
	e.stop();

	return 0;
}