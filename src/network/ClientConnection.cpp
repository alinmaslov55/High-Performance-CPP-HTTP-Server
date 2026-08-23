#include "http/network/ClientConnection.hpp"

#include <stdexcept>

#include <sys/socket.h>

namespace http{

ClientConnection::ClientConnection(Socket socket):
    socket_(std::move(socket)){}

std::string ClientConnection::receive(){
    char buffer[4096];

    const ssize_t bytes_received = ::recv(
        socket_.fd(),
        buffer,
        sizeof(buffer),
        0
    );

    if(bytes_received == -1){
        throw std::runtime_error("Failed to receive data");
    }

    if(bytes_received == 0){
        return "";
    }

    return std::string(
        buffer,
        static_cast<std::size_t>(bytes_received)
    );
}

void ClientConnection::send(const std::string& data){
    const char* buffer = data.data();
    std::size_t bytes_remaining = data.size();

    while(bytes_remaining > 0){
        const ssize_t bytes_sent = ::send(
            socket_.fd(),
            buffer,
            bytes_remaining,
            0
        );

        if(bytes_sent == -1){
            throw std::runtime_error("Failed to send data");
        }

        buffer += bytes_sent;
        bytes_remaining -= bytes_sent;
    }
}

} // namespace http