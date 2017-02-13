#include "../../include/xnet.hpp"
#include <fstream>
int main()
{
	xnet::proactor proactor;
	auto acceptor = proactor.get_acceptor();	
	proactor.set_timer(1000, [&] {
		return true;
	});
	xnet::msgbox<std::function<void()>> msgbox(proactor);
	proactor.run();
	return 0;
}
