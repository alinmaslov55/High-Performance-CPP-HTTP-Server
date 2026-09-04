#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "http/http/Router.hpp"
#include "http/network/Socket.hpp"
#include "http/network/Epoll.hpp"
#include "http/network/ClientConnection.hpp"
#include "http/concurrency/ThreadPool.hpp"

#include <unordered_map>
#include <mutex>

namespace http {
using namespace concurrency;

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
	void handleNewConnection();
	void handleClientData(int client_fd);
	void disconnectClient(int client_fd);
	void sweepIdleConnections();
	static constexpr int CONNECTION_TIMEOUT_SECONDS = 30;

	Socket server_socket_;
	int port_;
	const Router& router_;

	Epoll epoll_;
	std::unordered_map<int, std::shared_ptr<ClientConnection>> active_connections_;
	std::mutex connections_mutex_;
	ThreadPool thread_pool_;
};

} // namespace http

#endif // TCP_SERVER_HPP
