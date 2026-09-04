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

const char RESPONSE[] = "HTTP/1.1 200 OK\r\n"
						"Content-Type: text/plain\r\n"
						"Content-Length: 12\r\n"
						"Connection: close\r\n"
						"\r\n"
						"Hello World!";

TcpServer::TcpServer(int port, const Router& router)
	: server_socket_(Socket::create_tcp()),
	port_(port),
	router_(router),
	thread_pool_(
		std::thread::hardware_concurrency() > 0 ?
		std::thread::hardware_concurrency() : DEFAULT_THREADS
	)
{
	server_socket_.setReuseAddress();
	server_socket_.bind(port_);
	server_socket_.setNonBlocking();
	server_socket_.listen();

	epoll_.add(server_socket_.fd(), EPOLLIN | EPOLLET);
}

void TcpServer::start() {
	std::cout << "[INFO] Async Event Loop Started\n";

	while (true) {
		auto events = epoll_.wait(1000);

		for(const auto& event: events){
			if(event.data.fd == server_socket_.fd()){
				handleNewConnection();
				continue;
			}

			if(event.events & (EPOLLERR | EPOLLHUP)){
				disconnectClient(event.data.fd);
			}else if(event.events & EPOLLIN){
				int client_fd = event.data.fd;
				thread_pool_.enqueue([this, client_fd](){
					handleClientData(client_fd);
				});
			}
		}
		sweepIdleConnections();
	}
}

void TcpServer::handleNewConnection(){
	while(true){
		Socket client_socket = server_socket_.accept();

        if (!client_socket.valid()) {
            break;
        }

        int client_fd = client_socket.fd();
        client_socket.setNonBlocking();
        {
			std::unique_lock<std::mutex> lock(connections_mutex_);
			active_connections_[client_fd] = std::make_shared<ClientConnection>(std::move(client_socket));
		}

        epoll_.add(client_fd, EPOLLIN | EPOLLET);
	}
}

void TcpServer::handleClientData(int client_fd){
	connections_mutex_.lock();

	auto it = active_connections_.find(client_fd);
    if (it == active_connections_.end()){
		connections_mutex_.unlock();
		return;
	}

    std::shared_ptr<ClientConnection>& connection = it->second;
	connections_mutex_.unlock();

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
}

void TcpServer::disconnectClient(int client_fd){
	std::lock_guard<std::mutex> lock(connections_mutex_);
	epoll_.remove(client_fd);
	active_connections_.erase(client_fd);
}

void TcpServer::sweepIdleConnections() {
    std::vector<int> stale_fds;

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (const auto& [fd, client] : active_connections_) {
            if (client->isIdle(CONNECTION_TIMEOUT_SECONDS)) {
                stale_fds.push_back(fd);
            }
        }
    }


    for (int fd : stale_fds) {
        std::cout << "[LOG] Disconnecting idle client FD: " << fd << '\n';
        disconnectClient(fd);
    }
}

} // namespace http
