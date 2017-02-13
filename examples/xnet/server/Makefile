CFLAGS = -g -W \
-O3 \
-Wall \
-Wpointer-arith \
-Wshadow \
-Wno-long-long \
-Wformat \
-Winvalid-pch \
-std=c++11 \
-I ./../../include
run:server
	./server
server:rm_server
	g++ server.cpp $(CFLAGS) -o server
rm_server:
	$(shell if [ -f server ]; then rm server; fi;)

