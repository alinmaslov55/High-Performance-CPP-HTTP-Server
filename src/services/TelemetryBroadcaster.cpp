#include "http/services/TelemetryBroadcaster.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <algorithm>
#include <iostream>

namespace http {
namespace services {

TelemetryBroadcaster::TelemetryBroadcaster(TcpServer& server) : server_(server) {}

TelemetryBroadcaster::~TelemetryBroadcaster() {
    stop();
}

void TelemetryBroadcaster::addClient(const std::shared_ptr<ClientConnection>& client) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.push_back(client);
}

void TelemetryBroadcaster::start() {
    if (running_.load()) return;
    running_.store(true);
    worker_ = std::thread(&TelemetryBroadcaster::run, this);
}

void TelemetryBroadcaster::stop() {
    running_.store(false);
    if (worker_.joinable()) {
        worker_.join();
    }
}

void TelemetryBroadcaster::run() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        nlohmann::json metrics;
        metrics["active_connections"] = TcpServer::active_connections_.load(std::memory_order_relaxed);
        metrics["total_requests"] = TcpServer::total_requests_.load(std::memory_order_relaxed);
        metrics["queued_tasks"] = server_.getPendingTaskCount();

        std::string payload = metrics.dump();

        std::lock_guard<std::mutex> lock(mutex_);
        clients_.erase(
            std::remove_if(clients_.begin(), clients_.end(),
                [&payload](const std::weak_ptr<ClientConnection>& weak_conn) {
                    if (auto conn = weak_conn.lock()) {
                        conn->sendWebSocketMessage(payload);
                        return false;
                    }
                    return true;
                }),
            clients_.end()
        );
    }
}

} // namespace services
} // namespace http