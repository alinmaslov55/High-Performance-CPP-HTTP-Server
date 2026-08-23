#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <cstddef>
#include <string>

#include "http/network/Socket.hpp"

namespace http {

class ClientConnection{
public:
    explicit ClientConnection(Socket socket);

    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;

    ClientConnection(ClientConnection&&) noexcept = default;
    ClientConnection& operator=(ClientConnection&&) noexcept = default;

    ~ClientConnection() = default;

    [[nodiscard]]
    std::string receive();

    void send(const std::string& data);

private:
    Socket socket_;
};

} // namespace http


#endif // CLIENT_CONNECTION_HPP