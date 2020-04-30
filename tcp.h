#ifndef TCP_H
#define TCP_H

#include <atomic>

const int TCP_CONNECTION_TIMOUT = 15;

void tcpServer(const char* port, std::atomic_bool& control);
void tcpEcho(std::atomic_bool& control, int socket);

int tcpClient(const char* port);

#endif //TCP_H
