#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

namespace http{

class TcpServer{
public:
    explicit TcpServer(int port);

    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // [INFO] To be implemented if problems appear
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    void start();
private:
    void createSocket();
    void bindSocket();
    void listenSocket();

    int server_socket_;
    int port_;
};

} // namespace http

#endif // TCP_SERVER_HPP