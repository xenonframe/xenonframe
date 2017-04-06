#include "xactor/xactor.hpp"
#include "xutil/function_traits.hpp"
using namespace xactor;

struct hello 
{
	std::string data_;
	RegistActorMsg(hello, data_);
};

struct world
{
	hello hello_;
	RegistActorMsg(world, hello_);
};

class user :public actor
{
public:
	user(){ } 
	~user(){ } 
	void init()
	{
		using xutil::to_function;
		receive(to_function([this](const address& from, hello &&m) {

			std::cout << m.data_ << std::this_thread::get_id() << std::endl;
			send(from, world{ m });
		}));
	}
	void send_hello()
	{
		hello msg;
		msg.data_ = "hello world  ";
		send(get_address(), msg);
	}
};

class monitor : public actor
{
public:
	monitor(address addr)
		:user_(addr)
	{
	};
	~monitor() {}
	void init()
	{
		receive([this](const address & from, world &&w){

			send(from, w.hello_);
		});

		send(user_, hello{"hello world "});
	}
private:
	address user_;
};
int hello_world()
{
	xengine engine;

	engine.start();

	auto user_ = engine.spawn<user>();
	engine.spawn<monitor>(user_);

	getchar();
	engine.stop();

	return 0;
}