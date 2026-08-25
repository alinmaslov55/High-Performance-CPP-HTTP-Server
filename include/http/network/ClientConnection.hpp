#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <cstddef>
#include <string>
#include <string_view>

#include "http/network/ReadBuffer.hpp"
#include "http/network/Socket.hpp"

namespace http {

class ClientConnection {
  public:
	explicit ClientConnection(Socket socket);

	ClientConnection(const ClientConnection &) = delete;
	ClientConnection &operator=(const ClientConnection &) = delete;

	ClientConnection(ClientConnection &&) noexcept = default;
	ClientConnection &operator=(ClientConnection &&) noexcept = default;

	~ClientConnection() = default;

	[[nodiscard]]
	bool read();

	void send(const std::string &data);

	[[nodiscard]]
	std::string_view data() const noexcept;

  private:
	Socket socket_;
	ReadBuffer readBuffer_;
};

} // namespace http

#endif // CLIENT_CONNECTION_HPP