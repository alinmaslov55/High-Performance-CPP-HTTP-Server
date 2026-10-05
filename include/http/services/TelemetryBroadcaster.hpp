#ifndef TELEMETRY_BROADCASTER_HPP
#define TELEMETRY_BROADCASTER_HPP

#include "http/network/ClientConnection.hpp"
#include "http/network/TcpServer.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace http {
namespace services {

class TelemetryBroadcaster {
public:
    explicit TelemetryBroadcaster(TcpServer &server);
    ~TelemetryBroadcaster();

    TelemetryBroadcaster(const TelemetryBroadcaster &) = delete;
    TelemetryBroadcaster &operator=(const TelemetryBroadcaster &) = delete;

    void addClient(const std::shared_ptr<ClientConnection> &client);
    void start();
    void stop();

private:
    void run();

    TcpServer &server_;
    std::mutex mutex_;
    std::vector<std::weak_ptr<ClientConnection>> clients_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace services
} // namespace http

#endif // TELEMETRY_BROADCASTER_HPP