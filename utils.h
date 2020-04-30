#ifndef UTILS_H
#define UTILS_H

#include <sys/socket.h>
#include <atomic>

const int BUFFER_SIZE = 128;

void printError(const std::string& error);
void switchSocketToNonblockingMode(int sock);
int setTimeOut(int socketFd, u_int sec, u_int m_sec);

/* TCP */
int sendAll(int socket, const char* buf, u_int len, u_int flags = 0);
int receiveAll(int socket, char* buf, u_int &len, u_int flags = 0);

/* UDP */
int sendAllTo(int socket, const char* buf, u_int len, u_int flags, sockaddr* to, u_int toLength);
int receiveAllFrom(int socket, char* buf, u_int &len, u_int flags, sockaddr* from = nullptr, u_int* fromLength = nullptr);

int configureClient(const char* port, int socketType, int& socketFd, char* calculatedStrAddress);

void readInputKey(std::atomic_bool& control);
bool checkPort(const char* port);

void animation();

#endif //UTILS_H
