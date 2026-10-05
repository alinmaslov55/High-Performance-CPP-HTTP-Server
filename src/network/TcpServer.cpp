#include "http/network/TcpServer.hpp"
#include "http/http/HttpHeaders.hpp"
#include "http/network/ClientConnection.hpp"
#include "http/utils/Logger.hpp"
#include "http/utils/SslManager.hpp"
#include "http/utils/WebSocketUtils.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace http {

TcpServer::TcpServer(int port, const Router &router)
    : port_(port), router_(router),
      num_threads_(std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency()
                                                           : DEFAULT_THREADS),
      thread_pool_(DEFAULT_THREADS * 2) {}

void TcpServer::start() {
    LOG_INFO("Booting {} isolated Epoll loops (SO_REUSEPORT)...", num_threads_);
    LOG_INFO("Background ThreadPool ready for Database offloading.");

    for (int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this]() {
            Worker worker(port_, router_, thread_pool_);
            worker.run();
        });
    }

    for (auto &t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

TcpServer::Worker::Worker(int port, const Router &router, concurrency::ThreadPool &pool)
    : server_socket_(Socket::create_tcp()), router_(router), thread_pool_(pool) {
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

        for (const auto &event : events) {
            if (event.data.fd == server_socket_.fd()) {
                handleNewConnection();
            } else if (event.events & (EPOLLERR | EPOLLHUP)) {
                disconnectClient(event.data.fd);
            } else if (event.events & EPOLLIN) {
                handleClientData(event.data.fd);
            }
        }

        sweepIdleConnections();
    }

    LOG_INFO("Worker thread shutting down.");
}

void TcpServer::Worker::handleNewConnection() {
    while (true) {
        Socket client_socket = server_socket_.accept();

        if (!client_socket.valid()) {
            break;
        }

        int client_fd = client_socket.fd();
        client_socket.setNonBlocking();

        SSL_CTX *ctx = utils::SslManager::getInstance().getContext();
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);

        active_connections_[client_fd] =
            std::make_shared<ClientConnection>(std::move(client_socket), ssl, router_);

        TcpServer::active_connections_.fetch_add(1, std::memory_order_relaxed);

        epoll_.add(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
    }
}

void TcpServer::Worker::handleClientData(int client_fd) {
    auto it = active_connections_.find(client_fd);
    if (it == active_connections_.end()) {
        return;
    }

    std::shared_ptr<ClientConnection> &connection = it->second;

    try {
        connection->updateActivity();

        if (!connection->isHandshakeComplete()) {
            uint32_t next_epoll_events = 0;
            if (!connection->doHandshake(next_epoll_events)) {
                epoll_.modify(client_fd, next_epoll_events);
                return;
            }
        }

        if (!connection->read()) {
            disconnectClient(client_fd);
            return;
        }

        if (connection->isHttp2()) {
            connection->processHttp2();

            // Re-arm epoll for the next batch of multiplexed frames
            epoll_.modify(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
            return;
        }

        while (true) {
            if (connection->isWebSocket()) { // WEBSOCKET ROUTING (If connection upgraded)
                WebSocketFrame frame;
                ParseResult result = connection->parseWebSocketFrame(frame);

                if (result == ParseResult::Incomplete) {
                    epoll_.modify(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                    break;
                }

                if (result == ParseResult::Complete) {
                    if (frame.opcode == WebSocketOpcode::Close) {
                        LOG_INFO("Client FD {} gracefully closed WebSocket", client_fd);
                        disconnectClient(client_fd);
                        return;
                    }

                    // Delegate Text and Binary frames to the Router
                    if (frame.opcode == WebSocketOpcode::Text ||
                        frame.opcode == WebSocketOpcode::Binary) {
                        WebSocketFrame async_frame = std::move(frame);

                        thread_pool_.enqueue([this, connection, async_frame, client_fd]() mutable {
                            try {
                                router_.handleWs(connection, async_frame);

                                if (TcpServer::isRunning()) {
                                    epoll_.modify(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                                } else {
                                    ::shutdown(client_fd, SHUT_RDWR);
                                    ::close(client_fd);
                                }
                            } catch (const std::exception &e) {
                                LOG_ERROR("WS task failed for FD {}: {}", client_fd, e.what());
                                ::shutdown(client_fd, SHUT_RDWR);
                                ::close(client_fd);
                            }
                        });

                        return; // Exit the loop; the ThreadPool will re-arm epoll when finished
                    }

                    // Re-arm epoll for non-data frames (like Ping/Pong)
                    epoll_.modify(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                    continue;
                }

                // If result is Invalid (malformed WS frame), drop the connection
                if (result == ParseResult::Invalid) {
                    disconnectClient(client_fd);
                    return;
                }
            } else { // STANDARD HTTP ROUTING
                HttpRequest request;
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
                    std::string_view connection_hdr = request.header("Connection");
                    std::string_view upgrade_hdr = request.header("Upgrade");

                    // Intercept WebSocket Upgrades
                    if (HttpHeaders::equalsIgnoreCase(upgrade_hdr, "websocket") &&
                        connection_hdr.find("Upgrade") != std::string_view::npos) {

                        std::string_view client_key = request.header("Sec-WebSocket-Key");

                        if (!client_key.empty()) {
                            std::string accept_key =
                                utils::WebSocketUtils::generateAcceptKey(client_key);

                            // Building HTTP 101 Switching protocols response
                            HttpResponse ws_response;
                            ws_response.setStatus(HttpStatus::SwitchingProtocols);
                            ws_response.setHeader("Upgrade", "websocket");
                            ws_response.setHeader("Connection", "Upgrade");
                            ws_response.setHeader("Sec-WebSocket-Accept", accept_key);

                            connection->send(ws_response.serialize());
                            connection->consumeParsedRequest();

                            connection->upgradeToWebSocket(std::string(request.path()));
                            LOG_INFO("Upgraded FD {} to Secure WebSocket (wss://) on path: {}",
                                     client_fd, request.path());

                            epoll_.modify(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                            return; // Stop HTTP processing, wait for first WS frame
                        }
                    }

                    // Standard HTTP ThreadPool Offloading
                    bool keepAlive =
                        (request.version() == "HTTP/1.1") &&
                        !HttpHeaders::equalsIgnoreCase(request.header("Connection"), "close");

                    HttpRequest async_request = request;

                    connection->consumeParsedRequest();

                    TcpServer::total_requests_.fetch_add(1, std::memory_order_relaxed);

                    thread_pool_.enqueue([this, connection, async_request, client_fd,
                                          keepAlive]() mutable {
                        try {
                            auto start_time = std::chrono::steady_clock::now();
                            HttpResponse response = router_.handle(async_request);

                            auto end_time = std::chrono::steady_clock::now();
                            auto duration_ms =
                                std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                                      start_time)
                                    .count();

                            LOG_INFO("Responded [{}] to {} in {}ms",
                                     static_cast<int>(response.status()), async_request.path(),
                                     duration_ms);

                            response.setHeader("Connection", keepAlive ? "keep-alive" : "close");

                            connection->send(response.serialize());
                            connection->updateActivity();

                            if (keepAlive && TcpServer::isRunning()) {
                                epoll_.modify(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                            } else {
                                ::shutdown(client_fd, SHUT_RDWR);
                                ::close(client_fd);
                            }
                        } catch (const std::exception &e) {
                            LOG_ERROR("Async task failed for FD {}: {}", client_fd, e.what());
                            ::shutdown(client_fd, SHUT_RDWR);
                            ::close(client_fd);
                        }
                    });

                    return;
                }
            }
        }
    } catch (const std::exception &e) {
        LOG_ERROR("Exception while handling client FD {}: {}", client_fd, e.what());
        disconnectClient(client_fd);
    }
}

void TcpServer::Worker::disconnectClient(int client_fd) {
    epoll_.remove(client_fd);
    if (active_connections_.erase(client_fd) > 0) {
        TcpServer::active_connections_.fetch_sub(1, std::memory_order_relaxed);
    }
}

void TcpServer::Worker::sweepIdleConnections() {
    std::vector<int> stale_fds;

    for (const auto &[fd, client] : active_connections_) {
        if (client->isIdle(CONNECTION_TIMEOUT_SECONDS)) {
            stale_fds.push_back(fd);
        }
    }

    for (int fd : stale_fds) {
        LOG_DEBUG("Disconnecting idle client FD: {}", fd);
        disconnectClient(fd);
    }
}

void TcpServer::stop() {
    running_.store(false, std::memory_order_release);
}

bool TcpServer::isRunning() {
    return running_.load(std::memory_order_acquire);
}

} // namespace http
