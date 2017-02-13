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

client:rm
	g++ rpc_client.cpp $(CFLAGS) -o rpc_client 
rm:
	$(shell if [ -f rpc_client ]; then rm rpc_client; fi;)

