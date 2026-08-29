#include "http/network/TcpServer.hpp"
#include "http/http/Router.hpp"
#include <iostream>

using namespace http;

int main() {
    Router router;

    router.get("/", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;
        res.setStatus(HttpStatus::OK);
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Welcome to the custom C++ HTTP Server!");
        return res;
    });

    router.get("/api/users", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;
        res.setStatus(HttpStatus::OK);
        res.setHeader("Content-Type", "application/json");
        res.setBody("{\"users\": [\"Alice\", \"Bob\", \"Charlie\"]}");
        return res;
    });

    try {
        std::cout << "Starting server...\n";
        TcpServer server(8080, router);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Server crashed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}