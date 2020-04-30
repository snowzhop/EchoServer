#include <string>
#include <iostream>
#include <unordered_map>
#include <future>
#include <cstring>

#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <signal.h>

#include "tcp.h"
#include "utils.h"

const int MAX_EPOLL_EVENTS = 30;

void tcpServer(const char* port, std::atomic_bool& control) {
    int reuse = 1;
    addrinfo hints{0};
    addrinfo* serverInfo;
    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_PASSIVE;
    
    if (getaddrinfo(nullptr, port, &hints, &serverInfo) == -1) {
        printError(std::string("TCP server: ").append(strerror(errno)));
        return;
    }

    int listenSocketFd = -1;
    addrinfo* it;
    for (it = serverInfo; it != nullptr; it = it->ai_next) {
        listenSocketFd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (listenSocketFd == -1) {
            continue;
        }
        if (setsockopt(listenSocketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) != -1) {
            if (bind(listenSocketFd, serverInfo->ai_addr, serverInfo->ai_addrlen) != -1) {
                break;
            }
        }
        close(listenSocketFd);
    }

    if (it == nullptr) {
        printError(std::string("TCP server: server creating failed: ").append(strerror(errno)));
        return;
    }
    
    switchSocketToNonblockingMode(listenSocketFd);
    listen(listenSocketFd, 30);
    
    epoll_event ev;
    epoll_event events[MAX_EPOLL_EVENTS];
    
    int epfd = epoll_create(0xABC);
    if (epfd == -1) {
        printError(std::string("TCP server: ").append(strerror(errno)));
        return;
    }
    
    ev.events = EPOLLIN;
    ev.data.fd = listenSocketFd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenSocketFd, &ev) == -1) {
        printError(std::string("TCP server: ").append(strerror(errno)));
        return;
    }
    
    std::unordered_map<int, std::future<void>> futures;
    std::future<void> fut;
    
    while (control) {
        int numOfFds = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, 300);
        if (numOfFds == -1) {
            printError(std::string("TCP server: ").append(strerror(errno)));
            return;
        }
        
        for (int i = 0; i < numOfFds; ++i) {
            if (events[i].data.fd == listenSocketFd) {
                int connectionSocket = accept(listenSocketFd, nullptr, nullptr);
                
                ev.events = EPOLLRDHUP | EPOLLHUP;
                ev.data.fd = connectionSocket;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, connectionSocket, &ev) == -1) {
                    printError(std::string("TCP server: ").append(strerror(errno)));
                    return;
                }
                
                fut = std::async(std::launch::async, tcpEcho, std::ref(control), connectionSocket);
                futures.insert({connectionSocket, std::move(fut)});
            }
            if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                futures.erase(events[i].data.fd);
            }
        }
    }
    close(listenSocketFd);
}

void tcpEcho(std::atomic_bool& control, int socket) {
    char buffer[BUFFER_SIZE];
    
    int epollFd = epoll_create(0xFFF);
    if (epollFd == -1) {
        return;
    }
    
    setTimeOut(socket, TCP_CONNECTION_TIMOUT, 0);
    
    epoll_event ev{0};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = socket;
    
    epoll_ctl(epollFd, EPOLL_CTL_ADD, socket, &ev);
    
    while (control) {
        u_int bufferSize = BUFFER_SIZE;
        int nfds = epoll_wait(epollFd, &ev, 1, 300);
        if (nfds < 0) {
            break;
        }
        if (ev.events & EPOLLIN) {
            int ret = receiveAll(socket, buffer, bufferSize);
            if (ret == EAGAIN || ret == -1) {
                break;
            }
            
            for (int i = 0; i < bufferSize; ++i) {
                buffer[i] = toupper(buffer[i]);
            }
            
            sendAll(socket, buffer, bufferSize);
        }
    }
    close(socket);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, socket, nullptr);
}

int tcpClient(const char* port) {
    int mainSocketFd;
    char serverAddress[INET6_ADDRSTRLEN];
    
    int ret = configureClient(port, SOCK_STREAM, mainSocketFd, serverAddress);
    if (ret != 0) {
        return ret;
    }
    
    signal(SIGPIPE, SIG_IGN); /* Don't handle SIGPIPE signal */
    
    std::cout << "TCP: Connected to " << serverAddress << ":" << port << std::endl;
    
    char message[BUFFER_SIZE];
    
    std::cout << "message: ";
    while (std::cin.getline(message, BUFFER_SIZE, '\n')) {
        u_int length = strlen(message);
        ret = sendAll(mainSocketFd, message, length);
        if (ret != 0) {
            return ret;
        }
        memset(message, 0, BUFFER_SIZE);
        ret = receiveAll(mainSocketFd, message, length);
        if (ret == -1 || ret > 0) {
            return ret;
        }
        std::cout << "ECHO: " << message << "\nmessage: ";
    }
    
    return 0;
}