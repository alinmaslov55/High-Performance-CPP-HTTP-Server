#include "http/network/TcpServer.hpp"
#include "http/network/ClientConnection.hpp"
#include "http/http/HttpHeaders.hpp"
#include "http/utils/Logger.hpp"

#include <iostream>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace http {

TcpServer::TcpServer(int port, const Router& router)
	: port_(port),
	router_(router),
	num_threads_(std::thread::hardware_concurrency() > 0 ?
		std::thread::hardware_concurrency() : DEFAULT_THREADS),
        thread_pool_(DEFAULT_THREADS * 2)
{}

void TcpServer::start() {
    LOG_INFO("Booting {} isolated Epoll loops (SO_REUSEPORT)...", num_threads_);
    LOG_INFO("Background ThreadPool ready for Database offloading.");

    for (int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this]() {
            Worker worker(port_, router_, thread_pool_);
            worker.run();
        });
    }

    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

TcpServer::Worker::Worker(int port, const Router& router, concurrency::ThreadPool& pool)
    : server_socket_(Socket::create_tcp()),
      router_(router),
      thread_pool_(pool)
{
    server_socket_.setReuseAddress();
    server_socket_.setReusePort();
    server_socket_.bind(port);
    server_socket_.setNonBlocking();
    server_socket_.listen(128);

    epoll_.add(server_socket_.fd(), EPOLLIN | EPOLLET);
}

void TcpServer::Worker::run() {
    while (TcpServer::isRunning()) {
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

    LOG_INFO("Worker thread shutting down.");
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

                HttpRequest async_request = request;

                connection->consumeParsedRequest();

                thread_pool_.enqueue([this, connection, async_request, client_fd, keepAlive]() mutable {
                    try{
                        auto start_time = std::chrono::steady_clock::now();
                        HttpResponse response = router_.handle(async_request);

                        auto end_time = std::chrono::steady_clock::now();
                        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

                        LOG_INFO("Responded [{}] to {} in {}ms", 
                            static_cast<int>(response.status()), 
                            async_request.path(), 
                            duration_ms);

                        response.setHeader("Connection", keepAlive ? "keep-alive": "close");

                        connection->send(response.serialize());
                        connection->updateActivity();

                        if(keepAlive && TcpServer::isRunning()){
                            epoll_.modify(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                        } else {
                            ::shutdown(client_fd, SHUT_RDWR);
                            ::close(client_fd);
                        }
                    } catch (const std::exception& e){
                        LOG_ERROR("Async task failed for FD {}: {}", client_fd, e.what());
                        ::shutdown(client_fd, SHUT_RDWR);
                        ::close(client_fd);
                    }
                });

                return;
            }
        }
    }catch(const std::exception& e){
        LOG_ERROR("Exception while handling client FD {}: {}", client_fd , e.what());
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
        LOG_DEBUG("Disconnecting idle client FD: {}", fd);
        disconnectClient(fd);
    }
}

void TcpServer::stop(){
    running_.store(false, std::memory_order_release);
}

bool TcpServer::isRunning(){
    return running_.load(std::memory_order_acquire);
}

} // namespace http
