#include <iostream>
#include <limits>
#include <cstring>

#include "tcp.h"
#include "udp.h"

int main(int argc, char* argv[]) {
    char choice = 0;
    do {
        std::cout << "[T]CP or [U]DP? > ";
        choice = std::cin.get();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        if (choice == 'T' || choice == 't'
            || choice == 'U' || choice == 'u') {
            break;
        }
        else {
            std::cout << "Wrong choice!" << std::endl;
        }
    } while (true);
    
    std::cout << "Ctrl+[D] - close" << std::endl;
    int ret = 0;
    if (choice == 'T' || choice == 't') {
        ret = tcpClient("9999");
    }
    else {
        ret = udpClient("9999");
    }
    
    if (ret != 0) {
        std::cerr << "Error: " << strerror(ret) << std::endl;
    }
    else {
        std::cout << std::endl;
    }
}