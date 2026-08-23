#include "http/network/TcpServer.hpp"

#include <iostream>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "http/network/ClientConnection.hpp"

namespace http{

const char RESPONSE[] = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 12\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Hello World!";

TcpServer::TcpServer(int port):
    server_socket_(Socket::create_tcp()),
    port_(port)
{
    server_socket_.setReuseAddress();
    server_socket_.bind(port_);
    server_socket_.listen();
}

void TcpServer::start(){
    std::cout << "[INFO] Server listening on port " << port_ << '\n';

    while(true){
        ClientConnection connection(
            server_socket_.accept()
        );

        std::cout << "[INFO] Client Connected\n";

        const std::string request = connection.receive();

        if(request.empty()){
            continue;
        }

        std::cout << "[INFO] Request: " << request << '\n';
        
        connection.send(
            RESPONSE
        );
    }
}

} // namespace http