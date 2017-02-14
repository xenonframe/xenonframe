#include "xsocket.io/xsocket.io.hpp"

struct login
{
	int32_t numUsers;
	XGSON(numUsers);
};
struct stop_type
{
	std::string username;
	XGSON(username);
};
typedef stop_type typing;

struct new_message
{
	std::string username;
	std::string message;
	XGSON(username, message);
};
struct user_join
{
	std::string username;
	int32_t numUsers;
	XGSON(username, numUsers);
};

typedef  user_join user_left;

int main()
{

	int32_t sessions = 0;
	xsocket_io::xserver io;
	io.bind("127.0.0.1", 3001);
	io.set_static("static/");

	io.of("/chat")
		.on_connection([&](xsocket_io::socket &sock) {

		sock.on("add user", [&](xjson::obj_t &obj) {

			if (sock.get("add_user").size()) return;
			sock.set("add_user", "true");

			sessions++;
			sock.set("username", obj.get<std::string>());
			sock.emit("login", login{ sessions });
			sock.broadcast.emit("user joined", user_join{
				sock.get("username"),sessions
			});
		});

		sock.on("new message", [&](xjson::obj_t &obj) {
			new_message msg;
			msg.message = obj.get<std::string>();
			msg.username = sock.get("username");
			sock.broadcast.emit("new message", msg);
		});

		sock.on("stop typing", [&sock] {
			sock.broadcast.emit("stop typing", stop_type{
				sock.get("username")
			});
		});
		sock.on("typing", [&sock] {
			sock.broadcast.emit("typing", typing{
				sock.get("username")
			});
		});

		sock.on("disconnect", [&] {
			if (sock.get("add_user").size())
				sessions--;
			sock.broadcast.emit("user left", user_left{
				sock.get("username"), sessions
			});
		});
	});
	io.start();
	getchar();
}
