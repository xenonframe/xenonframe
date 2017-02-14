CFLAGS = -g -W \
-O3 \
-Wall \
-Wpointer-arith \
-Wshadow \
-Wno-long-long \
-Wformat \
-Winvalid-pch \
-std=c++1z \
-I ../../../src \
-l pthread

client:rm
	g++ main.cpp $(CFLAGS) -o rpc_client 
rm:
	$(shell if [ -f rpc_client ]; then rm rpc_client; fi;)

