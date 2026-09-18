#include "http/App.hpp"
#include "http/utils/Logger.hpp"
#include <iostream>

namespace http {

App::App(const AppConfiguration& config)
    : config_(config),
      mongo_instance_(),
      db_pool_(config_.db_uri),
      router_(),
      user_controller_(db_pool_),
      server_(config_.port, router_)
{
    router_.use([](HttpRequest& req, HttpResponse& res){
        LOG_INFO("Incoming request -> {}", req.path());
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