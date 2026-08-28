#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>

#include <cstdint>

namespace http {

/**
 * @brief RAII wrapper for socket file descriptor
 */
class Socket {
  private:
	explicit Socket(int file_descriptor) noexcept;
	Socket() noexcept;

  public:
	~Socket();
	Socket(const Socket &) = delete;
	Socket &operator=(const Socket &) = delete;

	Socket(Socket &&) noexcept;
	Socket &operator=(Socket &&) noexcept;

	/**
	 * @return The integer file descriptor
	 */
	[[nodiscard]]
	int fd() const noexcept;

	/**
	 * @brief Checks if the socket is currently valid
	 * @return true if the file descriptor is not -1, false otherwise
	 */
	[[nodiscard]]
	bool valid() const noexcept;
	/**
	 * @brief Closes the socket
	 */
	void close() noexcept;

	/**
	 * @brief Sets the SO_REUSEADDR socket option to allow rapid port rebinding
	 * @throws std::runtime_error
	 */
	void setReuseAddress();
	/**
	 * @brief Binds the socket to INADDR_ANY on the specified port
	 * @param port The port number to bind to
	 * @throws std::runtime_errpr
	 */
	void bind(uint16_t port);
	/**
	 * @brief Marks socket as listening
	 * @param backlog The port max length to which to which the queue of pending
	 * connection can grow
	 * @throws std::runtime_errpr
	 */
	void listen(int backlog = SOMAXCONN);
	/**
	 * @brief Blocks and accepts connection
	 * @throws std::runtime_errpr
	 */
	Socket accept();

	/**
	 * @brief Factory Method to create a new IPv4 TCP Socket
	 * @throws std::runtime_error
	 * @todo Eliminate throw std::runtime_error
	 */
	static Socket create_tcp();

  private:
	int file_descriptor_;
};

} // namespace http

#endif // SOCKET_HPP