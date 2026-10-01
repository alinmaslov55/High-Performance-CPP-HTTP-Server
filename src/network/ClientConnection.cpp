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

void ClientConnection::upgradeToWebSocket() {
	is_websocket_ = true;
	parser_.reset();
}

bool ClientConnection::isWebSocket() const {
	return is_websocket_;
}

ParseResult ClientConnection::parseWebSocketFrame(WebSocketFrame& out_frame) {
    std::string_view data = readBuffer_.data();

    // Needed at least 2 bytes just to read the header and length type
    if (data.size() < 2) {
        return ParseResult::Incomplete;
    }

    const auto* bytes = reinterpret_cast<const uint8_t*>(data.data());

    // Decode Byte 0
    bool fin = (bytes[0] & 0x80) != 0;
    uint8_t opcode = bytes[0] & 0x0F;

    // Decode Byte 1
    bool masked = (bytes[1] & 0x80) != 0;
    uint64_t payload_len = bytes[1] & 0x7F;

    size_t header_len = 2;

    // Calculate actual payload length and header size
    if (payload_len == 126) {
        if (data.size() < 4) return ParseResult::Incomplete;
        payload_len = (static_cast<uint64_t>(bytes[2]) << 8) | static_cast<uint64_t>(bytes[3]);
        header_len = 4;
    } else if (payload_len == 127) {
        if (data.size() < 10) return ParseResult::Incomplete;
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | static_cast<uint64_t>(bytes[2 + i]);
        }
        header_len = 10;
    }

    // Extract the masking key (Client to Server frames MUST be masked)
    uint8_t masking_key[4] = {0};
    if (masked) {
        if (data.size() < header_len + 4) return ParseResult::Incomplete;
        for (int i = 0; i < 4; ++i) {
            masking_key[i] = bytes[header_len + i];
        }
        header_len += 4;
    }

    // Ensure we have received the ENTIRE frame over TCP before processing
    if (data.size() < header_len + payload_len) {
        return ParseResult::Incomplete; 
    }

    // Extract and unmask the payload (XOR cipher)
    std::string payload;
    payload.resize(payload_len);
    for (size_t i = 0; i < payload_len; ++i) {
        payload[i] = bytes[header_len + i] ^ (masked ? masking_key[i % 4] : 0);
    }

    // Populate the output struct
    out_frame.fin = fin;
    out_frame.opcode = static_cast<WebSocketOpcode>(opcode);
    out_frame.payload = std::move(payload);

    // Consume the exact byte length of this frame from the network buffer
    readBuffer_.consume(header_len + payload_len);

    return ParseResult::Complete;
}

void ClientConnection::sendWebSocketMessage(const std::string& payload, WebSocketOpcode opcode) {
    std::string frame;
    const size_t len = payload.size();

    // Byte 0: FIN flag (0x80) OR'd with the Opcode
    frame.push_back(static_cast<char>(0x80 | static_cast<uint8_t>(opcode)));

    // Byte 1 + Length Extension
    if (len < 126) {
        // For length < 126, Byte 1 is just the length (MASK bit is 0)
        frame.push_back(static_cast<char>(len));
    } else if (len <= 65535) {
        // For length <= 64KB, Byte 1 is 126, followed by 2 bytes of length
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    } else {
        // For massive payloads, Byte 1 is 127, followed by 8 bytes of length
        frame.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<char>((len >> (i * 8)) & 0xFF));
        }
    }

    // Append the raw payload (Server-to-Client frames are NEVER masked)
    frame.append(payload);

    // Send it down the encrypted TLS socket!
    send(frame);
}

} // namespace http
