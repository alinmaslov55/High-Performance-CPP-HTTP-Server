#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>

#include <cstdint>

namespace http {

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

	[[nodiscard]]
	int fd() const noexcept;

	[[nodiscard]]
	bool valid() const noexcept;
	void close() noexcept;

	void setReuseAddress();
	void bind(uint16_t port);
	void listen(int backlog = SOMAXCONN);
	Socket accept();

	static Socket create_tcp();

  private:
	int file_descriptor_;
};

} // namespace http

#endif // SOCKET_HPP