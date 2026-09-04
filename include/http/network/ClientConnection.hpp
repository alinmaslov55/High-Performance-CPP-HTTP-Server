#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <chrono>

#include "http/http/HttpParser.hpp"
#include "http/http/HttpRequest.hpp"
#include "http/network/ReadBuffer.hpp"
#include "http/network/Socket.hpp"

namespace http {

/**
 * @brief Active session with a client
 */
class ClientConnection {
  public:
	explicit ClientConnection(Socket socket);

	ClientConnection(const ClientConnection &) = delete;
	ClientConnection &operator=(const ClientConnection &) = delete;

	ClientConnection(ClientConnection &&) noexcept = default;
	ClientConnection &operator=(ClientConnection &&) noexcept = default;

	~ClientConnection() = default;

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

	ParseResult parseRequest(HttpRequest &request);
	void consumeParsedRequest();

	void updateActivity();

	[[nodiscard]]
	bool isIdle(int timeoutSeconds) const;
  private:
	Socket socket_;
	ReadBuffer readBuffer_;
	HttpParser parser_;
	std::chrono::steady_clock::time_point lastActivity_ = std::chrono::steady_clock::now();
};

} // namespace http

#endif // CLIENT_CONNECTION_HPP
