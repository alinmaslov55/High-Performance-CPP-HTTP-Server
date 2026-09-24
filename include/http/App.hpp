#ifndef APP_HPP
#define APP_HPP

#include "http/network/TcpServer.hpp"
#include "http/http/Router.hpp"
#include "http/database/MongoPool.hpp"
#include "http/controllers/UserController.hpp"
#include "http/repositories/UserRepository.hpp"

#include <mongocxx/instance.hpp>

#include <string>

namespace http {

struct AppConfiguration {
    int port = 8080;
    std::string db_uri = "mongodb://localhost:27017";
};

class App {
public:
    explicit App(const AppConfiguration& config);
    ~App() = default;

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void run();
private:
    AppConfiguration config_;

    mongocxx::instance mongo_instance_;
    db::MongoPool db_pool_;
    Router router_;
    
    // Repositories
    repositories::UserRepository user_repo_;
    // Controller instances
    controllers::UserController user_controller_;

    TcpServer server_;
};

} // namespace http

#endif // APP_HPP