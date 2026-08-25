#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "http/network/Socket.hpp"

namespace http {

class TcpServer {
  public:
	explicit TcpServer(int port);

	~TcpServer() = default;

	TcpServer(const TcpServer &) = delete;
	TcpServer &operator=(const TcpServer &) = delete;

	// [INFO] To be implemented if problems appear
	TcpServer(TcpServer &&) = delete;
	TcpServer &operator=(TcpServer &&) = delete;

	void start();

  private:
	Socket server_socket_;
	int port_;
};

} // namespace http

#endif // TCP_SERVER_HPP