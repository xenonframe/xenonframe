#include "../../include/xnet.hpp"

int main()
{
	xnet::proactor proactor;

	auto connector = proactor.get_connector();

	connector.bind_fail_callback([&](std::string error_code) {
		std::cout << error_code.c_str() << std::endl;
		connector.close();
	});

	const std::string req = "hello world";
	
	std::map<int, xnet::connection> conns;
	int id = 0;
	auto conn_run = [&conns,&req](xnet::connection &&conn, int conn_id)
	{
		conns.emplace(conn_id, std::move(conn));
		conns[conn_id].regist_recv_callback([&conns,conn_id](char *data, std::size_t len)
		{
 			if (len == 0)
 			{
 				conns[conn_id].close();
 				conns.erase(conns.find(conn_id));
 				return;
 			}
			std::cout << (char*)data << std::endl;
 			conns[conn_id].close();
 			conns.erase(conns.find(conn_id));
			
		});

		conns[conn_id].regist_send_callback([&](std::size_t len)
		{
 			if (len == 0)
 			{
 				conns[conn_id].close();
 				conns.erase(conns.find(conn_id));
 				return;
 			}
		});
		conns[conn_id].async_send(req.c_str(), (int)req.size());
		conns[conn_id].async_recv_some();
		
	};

	connector.bind_success_callback([&](xnet::connection &&_conn) mutable 
	{
		conn_run(std::move(_conn), id);
		id++;
		connector.async_connect("192.168.0.9", 9001);
	});
	connector.async_connect("192.168.0.9", 9001);
	proactor.run();

	return  0;
}
