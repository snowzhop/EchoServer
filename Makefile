CC=g++

all: server client

server: server.o tcp.o udp.o utils.o
	$(CC) server.o tcp.o udp.o utils.o -o server.out -pthread
	
client: client.o tcp.o udp.o utils.o
	$(CC) client.o tcp.o udp.o utils.o -o client.out -pthread
	
server.o:
	$(CC) -c server.cpp -o server.o -std=c++11
	
client.o:
	$(CC) -c client.cpp -o client.o -std=c++11
	
tcp.o: tcp.cpp tcp.h
	$(CC) -c tcp.cpp -o tcp.o -std=c++11

udp.o: udp.cpp udp.h
	$(CC) -c udp.cpp -o udp.o -std=c++11
	
utils.o: utils.cpp utils.h
	$(CC) -c utils.cpp -o utils.o -std=c++11
	
clean:
	rm *.o
	rm *.out