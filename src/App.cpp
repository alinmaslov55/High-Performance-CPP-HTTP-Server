#include "http/App.hpp"
#include "http/utils/Logger.hpp"
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
      user_controller_(db_pool_),
      server_(config_.port, router_)
{
    std::string redis_uri = std::getenv("REDIS_URI") ? std::getenv("REDIS_URI") : "tcp://localhost:6379";
    auto redis_client = std::make_shared<sw::redis::Redis>(redis_uri);

    // Global Logging Middleware
    router_.use([](HttpRequest& req, HttpResponse& res){
        LOG_INFO("Incoming request -> {}", req.path());
        return true;
    });

    // Rate Limiting Middleware (Max 5 requests per 10 seconds)
    auto rateLimit = middlewares::RateLimiter::create(redis_client, 5, 10);

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
    LOG_INFO("MongoDB URI: {}", config_.db_uri);    server_.start();
    server_.start();
}

} // namespace http