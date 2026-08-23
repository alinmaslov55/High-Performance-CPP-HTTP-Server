#include "http/network/TcpServer.hpp"

#include <iostream>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace http{

TcpServer::TcpServer(int port):
    server_socket_(-1),
    port_(port)
{
    createSocket();
    bindSocket();
    listenSocket();
}

TcpServer::~TcpServer(){
    if(server_socket_ != -1){
        close(server_socket_);
    }
}

void TcpServer::createSocket(){
    server_socket_ = socket(
        AF_INET, // IPv4 127.0.0.1
        SOCK_STREAM, // TCP
        0
    );

    if(server_socket_ == -1){
        std::cerr << "[ERROR] Creating socket failed\n";
        throw std::runtime_error(
            "Failed to create server socket"
        );
    }

    int reuse = 1;

    if(setsockopt(
        server_socket_,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    ) == -1){
        close(server_socket_);

        server_socket_ = -1;

        throw std::runtime_error(
            "Failed to set SO_REUSEADDR"
        );
    }
}

void TcpServer::bindSocket(){
    sockaddr_in server_address{}; // IPv4 address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port_);
    


    if(bind(
        server_socket_,
        reinterpret_cast<sockaddr*>(&server_address),
        sizeof(server_address)
    ) == -1){
        std::cerr << "[ERROR] Binding socket failed\n";
        throw std::runtime_error(
            "Failed to bind server socket"
        );
    }
}

void TcpServer::listenSocket(){
    if(listen(server_socket_, SOMAXCONN) == -1){
        throw std::runtime_error(
            "Failed to listen on server socket"
        );
    }
}

void TcpServer::start(){
    std::cout << "[INFO] Server listening on port " << port_ << '\n';

    while(true){
        const int client_socket = accept(
            server_socket_,
            nullptr,
            nullptr
        );

        if(client_socket == -1){
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        std::cout << "Client connected\n";
        close(client_socket);
    }
}

} // namespace http