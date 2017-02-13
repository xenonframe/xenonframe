#pragma once
#include "xhttp_client.hpp"
namespace xhttp_client
{
	class client_maker
	{
	public:
		using client_callback = std::function<void(const std::string&, client &&)>;
		client_maker(xnet::proactor &pro)
			:pro_(pro)
		{

		}
		void get_client(const std::string &ip,int port, const client_callback &callback)
		{
			xcoroutine::create([=] {
				std::function<void()> resume;
				auto connector = pro_.get_connector();
				connector.bind_fail_callback([&](std::string error_code) {
					callback(error_code, client(xnet::connection()));
					resume();
				}).bind_success_callback([&](xnet::connection &&conn) {
					xcoroutine::create([&] {
						callback({}, client(std::move(conn)));
						resume();
					});
				}).async_connect(ip, port);
				xcoroutine::yield(resume);
			});
			
		}
	private:
		xnet::proactor &pro_;
	};
}