#ifndef UDP_H
#define UDP_H

#include <atomic>

void udpServer(const char* port, std::atomic_bool& control);
int udpClient(const char* port);

#endif //UDP_H
