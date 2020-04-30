all: server client

server: server.o tcp.o udp.o utils.o
	g++ server.o tcp.o udp.o utils.o -o server.out -pthread
	
client: client.o tcp.o udp.o utils.o
	g++ client.o tcp.o udp.o utils.o -o client.out -pthread
	
server.o:
	g++ -c server.cpp -o server.o -std=c++11
	
client.o:
	g++ -c client.cpp -o client.o -std=c++11
	
tcp.o: tcp.cpp tcp.h
	g++ -c tcp.cpp -o tcp.o -std=c++11

udp.o: udp.cpp udp.h
	g++ -c udp.cpp -o udp.o -std=c++11
	
utils.o: utils.cpp utils.h
	g++ -c utils.cpp -o utils.o -std=c++11
	
clean:
	rm *.o
	rm *.out