#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "http/http/Router.hpp"
#include "http/network/Socket.hpp"

namespace http {

/**
 * @brief Orchestrator for binding ports and accepting incoming connections
 */
class TcpServer {
  public:
	explicit TcpServer(int port, const Router& router);

	~TcpServer() = default;

	TcpServer(const TcpServer &) = delete;
	TcpServer &operator=(const TcpServer &) = delete;

	// [INFO] To be implemented if problems appear
	TcpServer(TcpServer &&) = delete;
	TcpServer &operator=(TcpServer &&) = delete;

	/**
	 * @brief Enters an infinite blocking loop to accept and handle client
	 * connections
	 */
	void start();

  private:
	Socket server_socket_;
	int port_;
	const Router& router_;
};

} // namespace http

#endif // TCP_SERVER_HPP