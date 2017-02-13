#include "../../include/proactor_pool.hpp"
#include <list>

int main()
{
	const char *rsp = "HTTP/1.1 200 OK\r\n"
		"Date: Fri, 28 Oct 2016 12:43:43 GMT\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: 11\r\n"
		"\r\n"
		"hello world";

	using namespace xnet;
	proactor_pool pp;
	std::mutex mtx;
	std::vector<std::pair<connection, std::list<std::string>>> connections;
	std::list<std::size_t> index_;
	connections.reserve(10000);

	pp.regist_accept_callback([&](connection &&conn) {
		std::lock_guard<std::mutex> lg(mtx);
		std::size_t index;
		connections.emplace_back(std::move(conn), std::list<std::string>());
		index = connections.size() - 1;

		connections[index].first.regist_recv_callback(
			[index,rsp,&connections,&index_](char *data, std::size_t len) {
			auto id = index;
			if (len == 0)
			{
				connections[id].first.close();
				return;
			}
			//std::cout << std::this_thread::get_id() << std::endl;
			if (connections[id].second.empty())
			{
				connections[id].first.async_send(rsp, (int)strlen(rsp));
				connections[id].second.emplace_back(rsp);
			}
			connections[id].first.async_recv_some();
		});
		connections[index].first.regist_send_callback(
			[index, &connections, &index_](std::size_t len) {
			auto id = index;
			if (len == 0)
			{
				connections[id].first.close();
			}
			connections[id].second.pop_front();
			if (connections[id].second.empty())
				return;
			connections[id].first.async_send(std::move(connections[id].second.front()));
		});
		connections[index].first.async_recv_some();

	}).start();
	pp.bind("0.0.0.0", 9001);
	getchar();
	pp.stop();

	return 0;
}