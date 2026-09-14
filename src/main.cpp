#include "http/App.hpp"
#include "http/network/TcpServer.hpp"

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

    AppConfiguration config{
        .port = 8080,
        .db_uri = "mongodb://localhost:27017"
    };

    App app(config);
    app.run();

    std::cout << "Server shutdown complete\n";
    return 0;
}
