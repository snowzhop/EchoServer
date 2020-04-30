#include <iostream>
#include <mutex>
#include <thread>
#include <regex>

#include <fcntl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "utils.h"

void printError(const std::string& error) {
    static std::mutex mu;
    mu.lock();
    std::cerr << error << std::endl;
    mu.unlock();
}

void switchSocketToNonblockingMode(int sock) {
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0) | O_NONBLOCK);
}

int setTimeOut(int socketFd, u_int sec, u_int m_sec) {
    timeval tv{0};
    tv.tv_sec = sec;
    tv.tv_usec = m_sec;
    return setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int sendAll(int socket, const char* buf, u_int len, u_int flags) {
    int total = 0;
    int bytesLeft = len; // int bytesLeft = len;
    int bytesSent = 0;
    char* temp = (char*)&len;
    if (len > 0) {
        send(socket, temp, 4, flags);
        while (total < len) {
            bytesSent = send(socket, buf + total, bytesLeft, flags);
            if (bytesSent == -1) { break; }
            total += bytesSent;
            bytesLeft -= bytesSent;
        }
    }
    
    return errno;
}

int receiveAll(int socket, char* buf, u_int &len, u_int flags) {
    u_int* bytesLeft = nullptr;
    int total = 0;
    int bytesRead = 0;
    char* temp = new char[4];
    
    recv(socket, temp, 4, flags);
    bytesLeft = (u_int*)temp;
    
    int difference = 0;
//    len = *bytesLeft;
    if (*bytesLeft > len) {
        difference = *bytesLeft - len;
        *bytesLeft = len;
    }
    else {
        len = *bytesLeft;
    }
    while(total < len) {
        bytesRead = recv(socket, buf + total, *bytesLeft, flags);
        if (bytesRead <= 0) { break; }
        total += bytesRead;
        *bytesLeft -= bytesRead;
    }
    
    if (difference != 0) {
        char* lostBuffer = new char[1500];
        std::cout << "Diff" << std::endl;
        while (difference != 0) {
            int lostBytes = recv(socket, lostBuffer, difference, flags);
            difference -= lostBytes;
        }
    }
    
    len = total;
    delete[] temp;
    
    return bytesRead == 0? -1 : errno;
}

//-------------------
/* Socktype == UDP */
//-------------------
int sendAllTo(int socket, const char* buf, u_int len, u_int flags, sockaddr* to, u_int toLength) {
    int total = 0;
    int bytesLeft = len;
    int bytesSent = 0;
    sendto(socket, &len, sizeof(len), 0, to, toLength);
    
    const int networkPacketLength = 64 * 1000;
    while (total < len) {
        int packetLength = 0;
        if (bytesLeft > networkPacketLength) {
            packetLength = networkPacketLength;
        }
        else {
            packetLength = bytesLeft;
        }
        bytesSent = sendto(socket, buf + total, packetLength, flags, to, toLength);
        if (bytesSent == -1) { break; }
        total += bytesSent;
        bytesLeft -= bytesSent;
    }
    
    return bytesSent == -1 ? errno : 0;
}

//-------------------
/* Socktype == UDP */
//-------------------
int receiveAllFrom(int socket, char* buf, u_int &len, u_int flags, sockaddr* from, u_int* fromLength) {
    u_int* bytesLeft = nullptr;
    int total = 0;
    int bytesRead = 0;
    char* temp = new char[sizeof(u_int)];
    
    recvfrom(socket, temp, sizeof(u_int), flags, from, fromLength);
    bytesLeft = reinterpret_cast<u_int*>(temp);
    
    len = *bytesLeft;
    int receivedLength = *bytesLeft;
    while (total < receivedLength) {
        bytesRead = recvfrom(socket, buf + total, *bytesLeft, flags, from, fromLength);
        if (bytesRead <= 0) { break; }
        total += bytesRead;
        *bytesLeft -= bytesRead;
    }
    
    delete[] temp;
    
    return bytesRead <= 0 ? errno: 0;
}

int configureClient(const char* port, int socketType, int& socketFd, char* calculatedStrAddress) {
    addrinfo hints{0};
    addrinfo *serverInfo;
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socketType;
    
    if (!checkPort(port)) {
        return -1;
    }
    
    if (getaddrinfo("127.0.0.1", port, &hints, &serverInfo) == -1) {
        return errno;
    }
    
    socketFd = -1;
    addrinfo* it;
    for (it = serverInfo; it != nullptr; it = it->ai_next) {
        if ((socketFd = socket(it->ai_family, it->ai_socktype, it->ai_protocol)) == -1) {
            continue;
        }
        if (connect(socketFd, it->ai_addr, it->ai_addrlen) != -1) {
            break;
        }
        close(socketFd);
    }
    
    if (it == nullptr) {
        return errno;
    }
    
    if (socketType == SOCK_DGRAM) {
        setTimeOut(socketFd, 1, 0);
        char testMessage[] = "test";
        int length = std::strlen(testMessage);
        sendAllTo(socketFd, testMessage, length, 0, nullptr, sizeof(sockaddr));
        memset(testMessage, 0, length);
        int ret = receiveAllFrom(socketFd, testMessage, reinterpret_cast<u_int&>(length), 0, nullptr, nullptr);
        if (ret > 0) {
            return ret;
        }
    }
    
    if (inet_ntop(
            it->ai_family,
            &(reinterpret_cast<sockaddr_in*>(it->ai_addr))->sin_addr,
            calculatedStrAddress,
            INET6_ADDRSTRLEN) == nullptr) {
        return errno;
    }
    
    freeaddrinfo(serverInfo);
    
    return 0;
}

void readInputKey(std::atomic_bool& control) {
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    
    newt.c_lflag = ~ (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    std::cin.get();
    control = false;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

bool checkPort(const char* port) {
    std::regex parsePort("([0-9]{1,4}|[1-5][0-9]"
                         "{4}|6[0-4][0-9]{3}|65[0-4]"
                         "[0-9]{2}|655[0-2][0-9]|6553[0-5])");
    std::smatch m;
    std::string port_(port);
    return regex_match(port_, m, parsePort);
}

void animation() {
    char loading = '\\';
    while (true) {
        switch (loading) {
            case '\\': {
                std::cout << '\r' << loading;
                std::cout.flush();
                loading = '|';
                break;
            }
            case '|': {
                std::cout << '\r' << loading;
                std::cout.flush();
                loading = '/';
                break;
            }
            case '/': {
                std::cout << '\r' << loading;
                std::cout.flush();
                loading = '-';
                break;
            }
            case '-': {
                std::cout << '\r' << loading;
                std::cout.flush();
                loading = '\\';
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}