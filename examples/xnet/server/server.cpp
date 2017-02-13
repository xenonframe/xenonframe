#include "xnet\xnet.hpp"
#include <fstream>
int main()
{
	xnet::proactor proactor;
	std::map<int, xnet::connection> conns;
	int id = 0;
	auto acceptor = proactor.get_acceptor();

	acceptor.regist_accept_callback(
		[&](xnet::connection &&conn)
	{
		conns.emplace(id, std::move(conn));
		
		conns[id].regist_recv_callback([id,&conns](char *data, std::size_t len)
		{
			if (len == 0)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}

			conns[id].async_send(
				"HTTP/1.1 200 OK\r\n"
				"Date: Fri, 28 Oct 2016 12:43:43 GMT\r\n"
				"Connection: keep-alive\r\n"
				"Content-Length: 11\r\n\r\n"
				"hello world"
			);

			conns[id].async_recv_some();
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
		id++;
	});
	proactor.set_timer(1000, [&] {
		return true;//repeat timer
	});

	acceptor.bind("0.0.0.0", 16504);
	proactor.run();

	return 0;
}
