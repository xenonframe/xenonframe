#include "../include/websocket.hpp"
#include "../../xtest/include/xtest.hpp"

xtest_run;
XTEST_SUITE(xserver)
{
	XUNIT_TEST(test)
	{
		std::map<int64_t, xwebsocket::session> sessions_;
		int64_t session_id_ = 0;
		int64_t msg_count_ = 0;
		xwebsocket::xserver server(1);
		server.bind("127.0.0.1", 9001).
			regist_accept_callback([&](xwebsocket::session &&sess) {

				sess.regist_frame_callback([session_id_, &sessions_, &msg_count_]
				(std::string &&payload, xwebsocket::frame_type type, bool){

					if (type == xwebsocket::frame_type::e_connection_close)
						return;
					++msg_count_;
					std::cout << payload << std::endl;
					sessions_[session_id_].send_text("hello world: " + std::to_string(msg_count_));
					//sessions_[session_id_].send_close();
				});
				sess.regist_close_callback([session_id_, &sessions_] {
					sessions_.erase(session_id_);
				});
			sessions_.emplace(session_id_, std::move(sess));
			session_id_++;
		});

		server.start();

		do
		{
		} while (getchar() != 'q');
	}
}