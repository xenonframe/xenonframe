#include "../../../xsimple_rpc/include/xsimple_rpc.hpp"
#include <iostream>
class client
{
	struct RPC
	{
		using result_t = std::pair<bool, std::string>;
		DEFINE_RPC_PROTO(get, result_t(std::string));
		DEFINE_RPC_PROTO(set, result_t(std::string, std::string));
		DEFINE_RPC_PROTO(del, result_t(std::string));
	};
public:
	client(const std::string &ip, int port)
		:ip_(ip), 
		port_(port)
	{
		rpc_proactor_pool_.start();
	}
	bool set(const std::string &key, const std::string &value)
	{
		if (!check_rpc())
			return false;
		try
		{
			auto result = rpc_client_->rpc_call<RPC::set>(key, value);
			if (result.first)
				return true;
			std::cout << "set '" << key << "' failed,error:" << result.second << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		return false;
	}
	bool get(const std::string &key, std::string &value)
	{
		if (!check_rpc())
			return false;
		try
		{
			auto result = rpc_client_->rpc_call<RPC::get>(key);
			if (result.first)
			{
				value = result.second;
				return true;
			}
			std::cout << "get '"<< key <<"' failed,error:" << result.second << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		return false;
	}
	bool del(const std::string &key)
	{
		if (!check_rpc())
			return false;
		try
		{
			auto result = rpc_client_->rpc_call<RPC::del>(key);
			if (result.first)
				return true;
			std::cout << "det '" << key << "' failed,error:" << result.second << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		return false;
	}
	bool connect()
	{
		try
		{
			rpc_client_.reset(new xsimple_rpc::client(rpc_proactor_pool_.connect(ip_, port_)));
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
			return false;
		}
		return true;
	}
private:
	bool check_rpc()
	{
		if (!rpc_client_)
			return connect();
		return !!rpc_client_;
	}

	std::string ip_;
	int port_;
	xsimple_rpc::rpc_proactor_pool rpc_proactor_pool_;
	std::shared_ptr<xsimple_rpc::client> rpc_client_;
};

int main(int args, char **argc)
{
	int port;
	std::cin >> port;
	client _client("127.0.0.1",port);
	if (!_client.connect())
		std::exit(1);
	do
	{
		std::cout << "get set del" << std::endl;
		std::string cmd;
		std::getline(std::cin, cmd);
		if (cmd == "set")
		{
			std::string key, value;
			std::getline(std::cin, key);
			std::getline(std::cin, value);
			std::cout << std::endl;
			std::cout << "..." << std::endl;
			if (_client.set(key, value))
				std::cout << cmd << "[" << key << ":" << value << "]" << std::endl;
		}
		else if (cmd == "get")
		{
			std::string key, value;
			std::getline(std::cin, key);
			std::cout << std::endl;
			std::cout << "..." << std::endl;
			if (_client.get(key, value))
				std::cout << cmd << "[" << key << ":" << value << "]" << std::endl;
		}
		else if (cmd == "del")
		{
			std::string key, value;
			std::getline(std::cin, key);
			std::cout << std::endl;
			std::cout << "..." << std::endl;
			if (_client.del(key))
				std::cout << cmd << "[" << key << "]" << std::endl;
		}
		else if (cmd == "q")
			return 0;
		else
		{
			std::size_t  count = std::strtoul(cmd.c_str(), 0, 10);
			if (count > 0)
			{
				std::getline(std::cin, cmd);
				if (cmd == "set")
				{
					for (size_t i = 0; i < count; i++)
					{
						if (_client.set(std::to_string(i), std::to_string(i)))
							std::cout << cmd << " [" << i << " : " << i << "]" << std::endl;
					}
				}
				else if (cmd == "get")
				{
					for (size_t i = 0; i < count; i++)
					{
						std::string value;
						if (_client.get(std::to_string(i), value))
							std::cout << cmd << " [" << i << " : " << value << "]" << std::endl;
					}
				}
				else if (cmd == "del")
				{
					for (size_t i = 0; i < count; i++)
					{
						if (_client.del(std::to_string(i)))
							std::cout << cmd << " [" << i << "]" << std::endl;
					}
					
				}
			}
		}
	} while (true);
	return 0;
}