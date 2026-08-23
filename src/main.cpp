#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(){
    const int server_socket = socket(
        AF_INET, // IPv4 127.0.0.1
        SOCK_STREAM, // TCP
        0
    );

    if(server_socket == -1){
        std::cerr << "[ERROR] Creating socket failed\n";
        return 1;
    }

    sockaddr_in server_address{}; // IPv4 address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);
    


    if(bind(
        server_socket,
        reinterpret_cast<sockaddr*>(&server_address),
        sizeof(server_address)
    ) == -1){
        std::cerr << "[ERROR] Binding socket failed\n";
        close(server_socket);
        return 1;
    }

    if(listen(server_socket, SOMAXCONN) == -1){
        std::cerr << "[ERROR] listen failed\n";
        close(server_socket);
        return 1;    
    }

    std::cout << "[INFO] Wainting for connection...\n";

    const int client_socket = accept(
        server_socket,
        nullptr,
        nullptr
    );

    if(client_socket == -1){
        std::cerr << "[ERROR] Failed to accept connection\n";
        close(server_socket);
        return 1; 
    }

    std::cout << "[INFO] Client connected\n";

    close(client_socket);
    close(server_socket);

    return 0;
}
