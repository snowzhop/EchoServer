#include <iostream>
#include <thread>

#include "tcp.h"
#include "udp.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    std::cout << "TCP/UDP server" << std::endl;
    std::cout << "Press any key to close" << std::endl;
    
    std::atomic_bool control(true);
    
    std::thread controlThread(readInputKey, std::ref(control));
    std::thread tcpServerThread(tcpServer, std::ref("9999"), std::ref(control));
    std::thread udpServerThread(udpServer, std::ref("9999"), std::ref(control));
    std::cout << "Ready..." << std::endl;
    
    std::thread animationThread(animation);
    animationThread.detach();
    
    controlThread.join();
    tcpServerThread.join();
    udpServerThread.join();
    
    std::cout << std::endl;
}