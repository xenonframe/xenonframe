#include "../../include/xnet.hpp"
#include <fstream>
int main()
{
	xnet::proactor proactor;
	std::map<int, xnet::connection> conns;
	int id = 0;
	auto acceptor = proactor.get_acceptor();
	std::fstream is;
	is.open("./rsp.txt");
	if (!is)
	{
		std::cout << "open file rsp.txt failed, exit()..." << std::endl;
		return 0;
	}
	std::string rsp((std::istreambuf_iterator<char>(is)),
		std::istreambuf_iterator<char>());

	acceptor.regist_accept_callback(
		[&](xnet::connection &&conn)
	{
		conns.emplace(id, std::move(conn));
		
		conns[id].regist_recv_callback([id,&rsp, &conns](char *data, std::size_t len)
		{
			if (len == 0)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}
			//std::cout << (char*)data << std::endl;;
			conns[id].async_send(rsp.data(), rsp.size());
			conns[id].async_recv_some();
			//conns[id].close();
			//std::cout << "conns :" << conns.size() << std::endl;
			//if(conns.find(id) != conns.end())
			//	conns.erase(conns.find(id));
		});
		conns[id].regist_send_callback([&](std::size_t len) 
		{
			if (len == 0)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}
		});
		conns[id].async_recv_some();
// 		conns[id].close();
// 		conns.erase(conns.find(id));
		id++;
	});
	proactor.set_timer(1000, [&] {
		return true;//repeat timer
	});

	acceptor.bind("0.0.0.0", 9001);
	proactor.run();

	return 0;
}
