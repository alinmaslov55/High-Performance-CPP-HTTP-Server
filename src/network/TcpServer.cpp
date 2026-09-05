#include "http/network/TcpServer.hpp"

#include <iostream>
#include <mutex>
#include <stdexcept>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "http/network/ClientConnection.hpp"
#include "http/http/HttpHeaders.hpp"

namespace http {

TcpServer::TcpServer(int port, const Router& router)
	: port_(port),
	router_(router),
	num_threads_(std::thread::hardware_concurrency() > 0 ?
		std::thread::hardware_concurrency() : DEFAULT_THREADS)
{}

void TcpServer::start() {
	std::cout << "[INFO] Booting " << num_threads_ << " isolated Epoll loops (SO_REUSEPORT)...\n";

    for (int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this]() {
            Worker worker(port_, router_);
            worker.run();
        });
    }

    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

TcpServer::Worker::Worker(int port, const Router& router)
    : server_socket_(Socket::create_tcp()),
      router_(router)
{
    server_socket_.setReuseAddress();
    server_socket_.setReusePort();
    server_socket_.bind(port);
    server_socket_.setNonBlocking();
    server_socket_.listen(128);

    epoll_.add(server_socket_.fd(), EPOLLIN | EPOLLET);
}

void TcpServer::Worker::run() {
    while (true) {
        auto events = epoll_.wait(1000);

        for (const auto& event : events) {
            if (event.data.fd == server_socket_.fd()) {
                handleNewConnection();
            } else if(event.events & (EPOLLERR | EPOLLHUP)) {
                disconnectClient(event.data.fd);
            } else if (event.events & EPOLLIN) {
                handleClientData(event.data.fd);
            }
        }

        sweepIdleConnections();
    }
}

void TcpServer::Worker::handleNewConnection(){
	while(true){
		Socket client_socket = server_socket_.accept();

        if (!client_socket.valid()) {
            break;
        }

        int client_fd = client_socket.fd();
        client_socket.setNonBlocking();
		active_connections_[client_fd] = std::make_shared<ClientConnection>(std::move(client_socket));
        epoll_.add(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
	}
}

void TcpServer::Worker::handleClientData(int client_fd){
	auto it = active_connections_.find(client_fd);
    if (it == active_connections_.end()){
		return;
	}

    std::shared_ptr<ClientConnection>& connection = it->second;

    try{
        connection->updateActivity();
        
        if (!connection->read()) {
            disconnectClient(client_fd);
            return;
        }
        
        HttpRequest request;
        while (true) {
            ParseResult result = connection->parseRequest(request);
            
            if (result == ParseResult::Incomplete) {
                epoll_.modify(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                break;
            }
            
            HttpResponse response;
            
            if (result == ParseResult::Invalid) {
                response.setStatus(HttpStatus::BadRequest);
                response.setBody("400 Bad Request");
                response.setHeader("Connection", "close");
                connection->send(response.serialize());
                disconnectClient(client_fd);
                return;
            }
            
            if (result == ParseResult::Complete) {
                bool keepAlive = (request.version() == "HTTP/1.1") &&
                !HttpHeaders::equalsIgnoreCase(request.header("Connection"), "close");

                response = router_.handle(request);

                response.setHeader("Connection", keepAlive ? "keep-alive" : "close");
                
                connection->send(response.serialize());
                connection->consumeParsedRequest();

                if (!keepAlive) {
                    disconnectClient(client_fd);
                    return;
                }
            }
        }
    }catch(const std::exception& e){
        std::cerr << "[ERROR] Exception while handling client FD " << client_fd << ": " << e.what() << '\n';
        disconnectClient(client_fd);
    }
}

void TcpServer::Worker::disconnectClient(int client_fd){
	epoll_.remove(client_fd);
	active_connections_.erase(client_fd);
}

void TcpServer::Worker::sweepIdleConnections() {
    std::vector<int> stale_fds;

    for (const auto& [fd, client] : active_connections_) {
        if (client->isIdle(CONNECTION_TIMEOUT_SECONDS)) {
            stale_fds.push_back(fd);
        }
    }

    for (int fd : stale_fds) {
        std::cout << "[LOG] Disconnecting idle client FD: " << fd << '\n';
        disconnectClient(fd);
    }
}

} // namespace http
