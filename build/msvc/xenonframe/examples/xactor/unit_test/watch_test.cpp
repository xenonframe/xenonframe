#include "xactor/xactor.hpp"
#include "xutil/function_traits.hpp"

namespace watch_test
{
	using namespace xactor;



	class user: public actor
	{
	public:
		void init()
		{
			set_timer(100, [this] {
				std::cout << "click." << std::endl;
				if (++click_ < 10)
					return true;
				close();
				return false;
			});
		}

		int click_ = 0;
	};

	class monitor: public actor
	{
	public:
		monitor(address addr)
			:user_(addr)
		{
		}
		~monitor()
		{

		}
	private:
		void regist_close_event()
		{
			regist([](const address &from, notify::actor_close && actor_close_){

				std::cout << "actor close: addrress ->"<<actor_close_.address_<< std::endl;
			});
		}
		void init()
		{
			regist_close_event();
			watch(user_);
		}
		address user_;
	};
}

int watch_test_test()
{
	xactor::xengine engine;
	engine.start();
	auto addr = engine.spawn<watch_test::user>();
	engine.spawn<watch_test::monitor>(addr);

	getchar();
	engine.stop();
	return 0;
}