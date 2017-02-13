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
run:async_rpc_client
	./async_rpc_client
server:async_rpc_client
	g++ async_rpc_client.cpp $(CFLAGS) -o async_rpc_client
rm_server:
	$(shell if [ -f async_rpc_client]; then rm async_rpc_client; fi;)

