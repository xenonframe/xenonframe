#include "../../include/xsimple_rpc.hpp"

using namespace xsimple_rpc;

struct MyStruct
{
	std::string hello;
	int world;
	std::vector<int> ints;

	XENDEC(hello, world, ints);

	std::string func(int, int) { return hello; }
	std::string func2() { return hello; }
	void func3()const { return; }
};

std::ostream & operator <<(std::ostream &out, const MyStruct &obj)
{
	out << "hello:" << obj.hello << " world: " << obj.world << " ints:[";
	for (auto &itr : obj.ints)
		out << ", " << itr;
	out << "]" << std::endl;
	return out;
}
struct rpc
{
	DEFINE_RPC_PROTO(add, int(int, int));
	DEFINE_RPC_PROTO(hello, std::string(const std::string &));
	DEFINE_RPC_PROTO(add_str, std::string(int, const std::string&));
	DEFINE_RPC_PROTO(func, std::string(int, int));
	DEFINE_RPC_PROTO(func2, std::string());
	DEFINE_RPC_PROTO(func3, void());
	DEFINE_RPC_PROTO(get_obj, MyStruct());
};

void test_rp_call(async_client *client)
{
	client->rpc_call<rpc::add>(std::forward_as_tuple(1, 1), [client](const std::string &, int resp) {
		std::cout << resp << std::endl;
		test_rp_call(client);
	});
}
int main()
{
	
	xnet::proactor proactor;
	auto connector = proactor.get_connector();

	async_client *client;
	connector.bind_success_callback([&](xnet::connection &&conn) 
	{
		client = new async_client(std::move(conn));
		test_rp_call(client);

		/*client->rpc_call<rpc::hello>(std::forward_as_tuple(std::string("hello")), 
			[](const std::string &, const std::string &resp) {
			std::cout << resp << std::endl;
		});

		client->rpc_call<rpc::add_str>(std::forward_as_tuple(520,std::string("-mama")),
			[](const std::string &, const std::string &resp) {
			std::cout << resp << std::endl;
		});

		client->rpc_call<rpc::func>(std::forward_as_tuple(10000,9999),
			[](const std::string &, const std::string &resp) {
			std::cout << resp << std::endl;
		});

		client->rpc_call<rpc::func2>(
			[](const std::string &, const std::string &resp) {
			std::cout << resp << std::endl;
		});

		client->rpc_call<rpc::func3>( [](const std::string &) {
			std::cout << "func3" << std::endl;
		});

		client->rpc_call<rpc::get_obj>([&](const std::string &, const MyStruct &obj) {
			std::cout << obj << std::endl;
			proactor.stop();
		});*/
	});
	connector.bind_fail_callback([](auto error_code) {
		std::cout << "connect failed error_code:"<<error_code << std::endl;
	});
	connector.async_connect("127.0.0.1", 9001);
	proactor.run();
}
