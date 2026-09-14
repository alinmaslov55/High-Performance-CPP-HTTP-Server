#include "http/network/TcpServer.hpp"
#include "http/http/Router.hpp"
#include "http/database/MongoPool.hpp"
#include "http/controllers/UserController.hpp"

#include <nlohmann/json.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <iostream>
#include <csignal>

using json = nlohmann::json;
using namespace http;


void handleSignal(int signal){
    std::cout << "[INFO] Signal " << signal << ". Stopping server\n";
    TcpServer::stop();
}

int main() {
    ::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);

    mongocxx::instance instance{};
    db::MongoPool db_pool("mongodb://localhost:27017");

    Router router;

    router.serveFiles("/static/", "./public");

    controllers::UserController user_controller(db_pool);
    user_controller.registerRoutes(router);

    std::cout << "Starting server on port 8080... (Press Ctrl+C to stop)\n";
    TcpServer server(8080, router);
    server.start();

    std::cout << "Server shutdown complete\n";
    return 0;
}
