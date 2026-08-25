#include "http/network/ClientConnection.hpp"

#include <stdexcept>

#include <sys/socket.h>

namespace http {

ClientConnection::ClientConnection(Socket socket)
	: socket_(std::move(socket)) {}

bool ClientConnection::read() {
	char temporary_buffer[8192];

	const ssize_t bytes_received =
		::recv(socket_.fd(), temporary_buffer, sizeof(temporary_buffer), 0);

	if (bytes_received == -1) {
		throw std::runtime_error("Failed to receive data");
	}

	if (bytes_received == 0) {
		return false;
	}

	readBuffer_.append(temporary_buffer,
					   static_cast<std::size_t>(bytes_received));

	return true;
}

void ClientConnection::send(const std::string &data) {
	const char *buffer = data.data();
	std::size_t bytes_remaining = data.size();

	while (bytes_remaining > 0) {
		const ssize_t bytes_sent =
			::send(socket_.fd(), buffer, bytes_remaining, 0);

		if (bytes_sent == -1) {
			throw std::runtime_error("Failed to send data");
		}

		buffer += bytes_sent;
		bytes_remaining -= bytes_sent;
	}
}

std::string_view ClientConnection::data() const noexcept {
	return readBuffer_.data();
}

} // namespace http