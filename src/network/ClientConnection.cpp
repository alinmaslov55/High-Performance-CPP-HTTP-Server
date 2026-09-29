#include <http/network/ClientConnection.hpp>

#include <cerrno>
#include <stdexcept>
#include <utility>

#include <sys/epoll.h>

namespace http {

ClientConnection::ClientConnection(Socket socket, SSL* ssl):
	socket_(std::move(socket)),
	ssl_(ssl),
	handshakeComplete_(false) {}

ClientConnection::~ClientConnection(){
	if(ssl_){
		SSL_shutdown(ssl_);
		SSL_free(ssl_);
	}
}

bool ClientConnection::read() {
	char temporaryBuffer[8192];

	while (true) {
		const int bytesReceived = SSL_read(ssl_, temporaryBuffer, sizeof(temporaryBuffer));
		
		if (bytesReceived > 0) {
			readBuffer_.append(temporaryBuffer,
							   static_cast<std::size_t>(bytesReceived));

			continue;
		}

		int err = SSL_get_error(ssl_, bytesReceived);
        
        if (err == SSL_ERROR_ZERO_RETURN) {
            return false; // Connection closed gracefully by client
        }

        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return true; // Non-blocking, try again later
        }

		throw std::runtime_error("SSL_read failed");
	}
}

void ClientConnection::send(const std::string &data) {
	const char *buffer = data.data();
	std::size_t bytesRemaining = data.size();

	while (bytesRemaining > 0) {
		const int bytesSent = SSL_write(ssl_, buffer, bytesRemaining);

		if (bytesSent > 0) {
			buffer += bytesSent;
			bytesRemaining -= static_cast<std::size_t>(bytesSent);

			continue;
		}

		int err = SSL_get_error(ssl_, bytesSent);
        
        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
            break; // Kernel buffer full, break
        }

        throw std::runtime_error("SSL_write failed");
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

void ClientConnection::updateActivity() {
	lastActivity_ = std::chrono::steady_clock::now();
}

bool ClientConnection::isIdle(int timeoutSeconds) const {
	auto now = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivity_).count();
	return duration > timeoutSeconds;
}

bool ClientConnection::doHandshake(uint32_t& out_epoll_events){
	if(handshakeComplete_) return true;

	int ret = SSL_accept(ssl_);
	if (ret == 1) {
        handshakeComplete_ = true;
        return true;
    }

	int err = SSL_get_error(ssl_, ret);
    if (err == SSL_ERROR_WANT_READ) {
        // OpenSSL needs more data from client. Wait for EPOLLIN.
        out_epoll_events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        return false;
    } else if (err == SSL_ERROR_WANT_WRITE) {
        // OpenSSL needs to write data to client. Wait for EPOLLOUT.
        out_epoll_events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
        return false;
    }

	throw std::runtime_error("SSL_accept failed");
}

} // namespace http
