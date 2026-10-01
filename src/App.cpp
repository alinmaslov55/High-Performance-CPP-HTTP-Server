#include "http/App.hpp"
#include "http/utils/Logger.hpp"
#include "http/utils/SslManager.hpp"
#include "http/middlewares/RateLimiter.hpp"

#include <sw/redis++/redis.h>

#include <iostream>
#include <cstdlib>

namespace http {

App::App(const AppConfiguration& config)
    : config_(config),
      mongo_instance_(),
      db_pool_(config_.db_uri),
      router_(),
      user_repo_(db_pool_),
      user_controller_(user_repo_),
      server_(config_.port, router_)
{
    const char* env_public_dir = std::getenv("PUBLIC_DIR_PATH");
    std::string public_dir = env_public_dir ? env_public_dir : "public";

    router_.serveFiles("/", public_dir);
    LOG_INFO("Serving static files from: {}", public_dir);

    std::string redis_uri = std::getenv("REDIS_URI") ? std::getenv("REDIS_URI") : "tcp://localhost:6379";
    auto redis_client = std::make_shared<sw::redis::Redis>(redis_uri);

    router_.ws("/api/chat", [](std::shared_ptr<http::ClientConnection>& conn, const http::WebSocketFrame& frame) {
        if (frame.opcode == http::WebSocketOpcode::Text) {
            LOG_INFO("Controller intercepted WS message: {}", frame.payload);
            
            conn->sendWebSocketMessage("Hello from the specialized WS Controller");
        }
    });

    // Global Logging Middleware
    router_.use([](HttpRequest& req, HttpResponse& res){
        LOG_INFO("Incoming request -> {}", req.path());
        return true;
    });

    // Rate Limiting Middleware (Max 5 requests per 10 seconds)
    auto rateLimit = middlewares::RateLimiter::create(redis_client, 100000, 10);

    router_.use([rateLimit](HttpRequest& req, HttpResponse& res) {
        if (req.path() == "/api/login") {
            return rateLimit(req, res);
        }
        return true;
    });
    user_controller_.registerRoutes(router_);
}

void App::run() {
    LOG_INFO("Application running on port {}", config_.port);
    LOG_INFO("MongoDB URI: {}", config_.db_uri);

    const char* env_cert = std::getenv("SSL_CERT_PATH");
    const char* env_key = std::getenv("SSL_KEY_PATH");

    std::string cert_path = env_cert ? env_cert : "certs/server.crt";
    std::string key_path  = env_key ? env_key : "certs/server.key";

    try {
        utils::SslManager::getInstance().initialize(cert_path, key_path);    } catch (const std::exception& e){
        LOG_ERROR("Failed to initialize TLS: {}", e.what());
        return; // Halt boot if SSL fails
    }

    server_.start();
}

} // namespace http
