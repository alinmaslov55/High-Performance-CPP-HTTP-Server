#include <http/network/ClientConnection.hpp>

#include <cerrno>
#include <stdexcept>
#include <utility>

#include <sys/socket.h>

namespace http {

ClientConnection::ClientConnection(Socket socket)
	: socket_(std::move(socket)) {}

bool ClientConnection::read() {
	char temporaryBuffer[8192];

	while (true) {
		const ssize_t bytesReceived =
			::recv(socket_.fd(), temporaryBuffer, sizeof(temporaryBuffer), 0);

		if (bytesReceived > 0) {
			readBuffer_.append(temporaryBuffer,
							   static_cast<std::size_t>(bytesReceived));

			return true;
		}

		if (bytesReceived == 0) {
			return false;
		}

		if (errno == EINTR) {
			continue;
		}

		throw std::runtime_error("Failed to receive data");
	}
}

void ClientConnection::send(const std::string &data) {
	const char *buffer = data.data();
	std::size_t bytesRemaining = data.size();

	while (bytesRemaining > 0) {
		const ssize_t bytesSent =
			::send(socket_.fd(), buffer, bytesRemaining, 0);

		if (bytesSent > 0) {
			buffer += bytesSent;
			bytesRemaining -= static_cast<std::size_t>(bytesSent);

			continue;
		}

		if (bytesSent == 0) {
			throw std::runtime_error("Socket send returned zero");
		}

		if (errno == EINTR) {
			continue;
		}

		throw std::runtime_error("Failed to send data");
	}
}

std::string_view ClientConnection::data() const noexcept {
	return readBuffer_.data();
}

ParseResult ClientConnection::parseRequest(HttpRequest &request) {
	return parser_.parse(readBuffer_.data(), request);
}

void ClientConnection::consumeParsedRequest() {
	const std::size_t consumed = parser_.consumedBytes();

	readBuffer_.consume(consumed);

	parser_.reset();
}

} // namespace http