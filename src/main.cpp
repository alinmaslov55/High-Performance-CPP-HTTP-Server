#include "http/App.hpp"
#include "http/network/TcpServer.hpp"
#include "http/utils/EnvParser.hpp"
#include "http/utils/Logger.hpp"

#include <iostream>
#include <csignal>
#include <cstdlib>

using json = nlohmann::json;
using namespace http;


void handleSignal(int signal){
    LOG_INFO("Signal {}. Stopping server", signal);
    TcpServer::stop();
}

int main() {
    utils::Logger::init();
    utils::EnvParser::load(".env");

    ::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);

    const char* env_port = std::getenv("PORT");
    const char* env_mongo = std::getenv("MONGO_URI");

    AppConfiguration config{
        .port = env_port ? std::atoi(env_port): 8080,
        .db_uri = env_mongo? std::string(env_mongo): "mongodb://localhost:27017"
    };

    App app(config);
    app.run();

    LOG_INFO("Server shutdown complete");
    return 0;
}
