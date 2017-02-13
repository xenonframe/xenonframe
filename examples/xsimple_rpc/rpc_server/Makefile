CFLAGS = -g -W \
-O3 \
-Wall \
-Wpointer-arith \
-Wshadow \
-Wno-long-long \
-Wformat \
-Winvalid-pch \
-std=c++1z \
-I ./../../include \
-l pthread
run:server
	./rpc_server
server:rm_server
	g++ rpc_server.cpp $(CFLAGS) -o rpc_server 
rm_server:
	$(shell if [ -f rpc_server ]; then rm rpc_server; fi;)

