#include <iostream>

#include "http/network/TcpServer.hpp"

int main(){
    try {
        http::TcpServer tcpServer(9090);

        tcpServer.start();
    } catch (const std::exception& e){
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }

    return 0;
}
