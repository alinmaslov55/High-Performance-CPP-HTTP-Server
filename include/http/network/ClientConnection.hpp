#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <chrono>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "http/http/HttpParser.hpp"
#include "http/http/HttpRequest.hpp"
#include "http/network/ReadBuffer.hpp"
#include "http/network/Socket.hpp"

namespace http {

enum class WebSocketOpcode : uint8_t {
	Continuation = 0x0,
	Text = 0x1,
	Binary = 0x2,
	Close = 0x8,
	Ping = 0x9,
	Pong = 0xA
};

struct WebSocketFrame {
	bool fin;
	WebSocketOpcode opcode;
	std::string payload;
};

/**
 * @brief Active session with a client
 */
class ClientConnection {
  public:
	explicit ClientConnection(Socket socket, SSL* ssl);

	ClientConnection(const ClientConnection &) = delete;
	ClientConnection &operator=(const ClientConnection &) = delete;

	ClientConnection(ClientConnection &&) noexcept = default;
	ClientConnection &operator=(ClientConnection &&) noexcept = default;

	~ClientConnection();

	bool doHandshake(uint32_t& out_epoll_events);
	bool isHandshakeComplete() const { return handshakeComplete_; }

	/**
	 * @brief Reads data from socket and appends to an internal buffer
	 * @throws std::runtime_error on read error
	 * @return true if data was read successfully, flas eif connection was
	 * closed by peer
	 */
	[[nodiscard]]
	bool read();

	/**
	 * @brief Sends a string over the socket
	 * @param data payload to be sent
	 * @throws std::runtime_error
	 */
	void send(const std::string &data);

	/**
	 * @brief Retrieves a read-only wrapper over the data from ReadBuffer
	 * @return std::string_view with the raw bytes
	 */
	[[nodiscard]]
	std::string_view data() const noexcept;

	// HTTP Parsing
	ParseResult parseRequest(HttpRequest &request);
	void consumeParsedRequest();

	// WebSocket State and Parsing
	void upgradeToWebSocket();
	[[nodiscard]]
	bool isWebSocket() const;
	ParseResult parseWebSocketFrame(WebSocketFrame& out_frame);
	void sendWebSocketMessage(const std::string& payload, WebSocketOpcode opcode = WebSocketOpcode::Text);

	void updateActivity();

	[[nodiscard]]
	bool isIdle(int timeoutSeconds) const;
  private:
	Socket socket_;
	SSL* ssl_;
	bool handshakeComplete_;
	bool is_websocket_{false};

	ReadBuffer readBuffer_;
	HttpParser parser_;
	std::chrono::steady_clock::time_point lastActivity_ = std::chrono::steady_clock::now();
};

} // namespace http

#endif // CLIENT_CONNECTION_HPP
