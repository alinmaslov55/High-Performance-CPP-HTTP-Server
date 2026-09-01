#include "http/network/Socket.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdexcept>

namespace http {

Socket::Socket() noexcept : file_descriptor_(-1) {}

Socket::Socket(int file_descriptor) noexcept
	: file_descriptor_(file_descriptor) {}

Socket::~Socket() { close(); }

Socket::Socket(Socket &&other) noexcept
	: file_descriptor_(other.file_descriptor_) {
	other.file_descriptor_ = -1;
}

Socket &Socket::operator=(Socket &&other) noexcept {
	if (this != &other) {
		close();

		file_descriptor_ = other.file_descriptor_;

		other.file_descriptor_ = -1;
	}

	return *this;
}

int Socket::fd() const noexcept { return file_descriptor_; }

bool Socket::valid() const noexcept { return file_descriptor_ != -1; }

void Socket::close() noexcept {
	if (file_descriptor_ != -1) {
		::close(file_descriptor_);

		file_descriptor_ = -1;
	}
}

Socket Socket::create_tcp() {
	const int file_descriptor = socket(AF_INET, SOCK_STREAM, 0);

	if (file_descriptor == -1) {
		throw std::runtime_error("Failed to create TCP socket");
	}

	return Socket(file_descriptor);
}

void Socket::setReuseAddress() {
	int reuse = 1;
	if (setsockopt(file_descriptor_, SOL_SOCKET, SO_REUSEADDR, &reuse,
				   sizeof(reuse)) == -1) {
		throw std::runtime_error("Failed to set SO_REUSEADDR");
	}
}

void Socket::bind(uint16_t port) {
	sockaddr_in address{};

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (::bind(file_descriptor_, reinterpret_cast<sockaddr *>(&address),
			   sizeof(address)) == -1) {
		throw std::runtime_error("Failed to bind socket");
	}
}

void Socket::listen(int backlog) {
	if (::listen(file_descriptor_, backlog) == -1) {
		throw std::runtime_error("Failed to listen on socket");
	}
}

Socket Socket::accept() {
	const int client_fd = ::accept(file_descriptor_, nullptr, nullptr);

	if (client_fd == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK){
			return Socket(); // socket with fd = -1
		}

		throw std::runtime_error("Failed to accept connection");
	}

	return Socket(client_fd);
}

void Socket::setNonBlocking(){
	int flags = fcntl(file_descriptor_, F_GETFL, 0);

	if(flags = -1){
		throw std::runtime_error("Failed to get socket flags");
	}

	if(fcntl(file_descriptor_, F_GETFL, flags | O_NONBLOCK) == -1){
		throw std::runtime_error("Failed to set non-blocking flag");
	}
}

} // namespace http