#include <string>
#include <limits>
#include <cstring>

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>

#include "udp.h"
#include "utils.h"

void udpServer(const char* port, std::atomic_bool& control) {
    addrinfo hints{0};
    addrinfo* serverInfo;
    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_DGRAM;
    hints.ai_flags      = AI_PASSIVE;
    
    getaddrinfo(nullptr, port, &hints, &serverInfo);
    
    int mainSocketFd = -1;
    addrinfo* it;
    for (it = serverInfo; it != nullptr; it = it->ai_next) {
        mainSocketFd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (mainSocketFd == -1) {
            continue;
        }
        if (bind(mainSocketFd, it->ai_addr, it->ai_addrlen) != -1) {
            break;
        }
        close(mainSocketFd);
    }
    
    if (it == nullptr) {
        printError(std::string("UDP server: server creating failed: ").append(strerror(errno)));
        return;
    }
    
    timeval tv{0};
    tv.tv_sec = 0;
    tv.tv_usec = 300000;
    if (setsockopt(mainSocketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        printError(std::string("UDP server: ").append(strerror(errno)));
        return;
    }
    
    char buffer[100];
    int prevRet = -20;
    sockaddr client;
    while (control) {
        u_int len = sizeof(sockaddr);
        u_int bufferSize;
        int ret = receiveAllFrom(mainSocketFd, buffer, bufferSize, 0, &client, &len);
//        int ret = receiveAll(mainSocketFd, buffer, bufferSize);
        if (ret == 0) {
            for (int i = 0; i < bufferSize; ++i) {
                buffer[i] = toupper(buffer[i]);
            }
            
            sendAllTo(mainSocketFd, buffer, bufferSize, 0, &client, len);
        }
    }
    close(mainSocketFd);
}

int udpClient(const char* port) {
    int mainSocketFd;
    char serverAddress[INET6_ADDRSTRLEN];
    int ret = configureClient(port, SOCK_DGRAM, mainSocketFd, serverAddress);
    
    if (ret != 0) {
        return ret;
    }
    
    std::cout << "UDP: Connected to " << serverAddress << ":" << port << std::endl;
    
    char message[BUFFER_SIZE];
    std::cout << "message: ";
    while (std::cin.getline(message, BUFFER_SIZE, '\n')) {
        u_int len = strlen(message);
        sendAllTo(mainSocketFd, message, len, 0, nullptr, sizeof(sockaddr));
        memset(message, 0, BUFFER_SIZE);
        ret = receiveAllFrom(mainSocketFd, message, len, 0, nullptr, nullptr);
        if (ret > 0) {
            return ret;
        }
        else {
            std::cout << "ECHO: " << message << "\nmessage: ";
        }
    }
    close(mainSocketFd);
    
    return 0;
}