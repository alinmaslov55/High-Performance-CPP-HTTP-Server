#include <iostream>

#include "http/network/TcpServer.hpp"


const char RESPONSE[] = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 12\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Hello World!";

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
