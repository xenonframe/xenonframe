CFLAGS = -g -W \
-O3 \
-Wall \
-Wpointer-arith \
-Wshadow \
-Wno-long-long \
-Wformat \
-Winvalid-pch \
-std=c++1z \
-I ../../src\
-l pthread
server:rm_server
	g++ main.cpp $(CFLAGS) -o xhttp_server
rm_server:
	$(shell if [ -f server ]; then rm server; fi;)

