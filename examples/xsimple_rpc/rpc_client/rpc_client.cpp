#include "../../include/xsimple_rpc.hpp"


struct MyStruct
{
	std::string hello;
	int world;
	std::vector<int> ints;

	XENDEC(hello, world, ints);

	std::string func(int, int){ return hello; }
	std::string func2(){ return hello; }
	void func3()const{ return; }
};

std::ostream & operator <<(std::ostream &out, MyStruct &obj)
{
	out << "hello:" << obj.hello << " world: " << obj.world << " ints:[";
	for (auto &itr : obj.ints)
		out <<", "<< itr;
	out << "]" << std::endl;
	return out;
}

int main()
{
	xsimple_rpc::rpc_proactor_pool rpc_proactor;
	rpc_proactor.start();


	
	try
	{
		//define rpc
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

		auto client = rpc_proactor.connect("127.0.0.1", 9001, 0);
		do
		{
			/*std::cout << "add: " << client.rpc_call<rpc::add>(1, 2) << std::endl;
			std::cout << "hello: " << client.rpc_call<rpc::hello>("hello") << std::endl;;
			std::cout << "add_str: " << client.rpc_call<rpc::add_str>(12345, "hello") << std::endl;;
			std::cout << "func: " << client.rpc_call<rpc::func>(2, 3) << std::endl;;
			std::cout << "func2: " << client.rpc_call<rpc::func2>() << std::endl;;
			std::cout << "func3: "; (client.rpc_call<rpc::func3>(), std::cout << std::endl);
			auto obj = client.rpc_call<rpc::get_obj>();
			std::cout << "get_obj: " << obj << std::endl;*/

			client.rpc_call<rpc::add>(1, 2);
			client.rpc_call<rpc::hello>("hello");
			client.rpc_call<rpc::add_str>(12345, "hello");
			client.rpc_call<rpc::func>(2, 3);
			client.rpc_call<rpc::func2>();
			client.rpc_call<rpc::func3>();
			client.rpc_call<rpc::get_obj>();

		} while (true);
		//auto client = rpc_proactor.connect("127.0.0.1", 9001, 0);
		
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	getchar();
	return 0;
}
