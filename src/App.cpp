#include "http/App.hpp"
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
    user_controller_.registerRoutes(router_);
}

void App::run() {
    std::cout << "[INFO] Application running on port " << config_.port << "\n";
    std::cout << "[INFO] MongoDB URI: " << config_.db_uri << "\n";
    server_.start();
}

} // namespace http