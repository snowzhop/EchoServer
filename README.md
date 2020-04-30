## Simple ECHO server

* Simple multithreaded TCP/UDP server.
* Users can connect to server using one client.
* User have to choose protocol before sending messages.

* Epoll API was used to organize multiple TCP connections.
* UDP server uses one thread, because a datagram socket doesn't need to be connected to another socket in order to be used.
