#include "http/network/TcpServer.hpp"
#include "http/http/Router.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;
using namespace http;

int main() {
    Router router;

    router.serveFiles("/static/", "./public");
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

    router.post("/api/users", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;

        try {
            json requestJson = json::parse(req.body());

            std::string name = requestJson.value("name", "Unknown");
            int age = requestJson.value("age", 0);

            json responseJson;
            responseJson["status"] = "success";
            responseJson["message"] = "User " + name + " created!";
            responseJson["data"] = {
                {"name", name},
                {"age", age},
                {"id", 42}
            };

            res.setStatus(HttpStatus::Created);
            res.json(responseJson.dump());

        } catch (const json::parse_error& e) {
            json errorJson;
            errorJson["status"] = "error";
            errorJson["message"] = "Invalid JSON payload";

            res.setStatus(HttpStatus::BadRequest);
            res.json(errorJson.dump());
        }

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