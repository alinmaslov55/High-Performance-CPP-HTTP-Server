#include "http/network/TcpServer.hpp"
#include "http/http/Router.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;
using namespace http;

std::string methodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

int main() {
    Router router;

    router.serveFiles("/static/", "./public");

    router.use([&](HttpRequest& req, HttpResponse& res) -> bool {
        std::cout << "[LOG] " << methodToString(req.method()) << " " 
                  << req.path() << '\n';
        
        return true;
    });

    router.use([](HttpRequest& req, HttpResponse& res) -> bool {
        std::string_view contentType = req.header("Content-Type");
        
        if (contentType.find("application/json") != std::string_view::npos) {
            if (!req.body().empty()) {
                try {
                    req.setJson(json::parse(req.body()));
                } catch (const json::parse_error& e) {
                    json errorJson;
                    errorJson["status"] = "error";
                    errorJson["message"] = "Invalid JSON payload";

                    res.setStatus(HttpStatus::BadRequest);
                    res.setHeader("Content-Type", "application/json");
                    res.setBody(errorJson.dump());
                    
                    return false;
                }
            }
        }
        return true;
    });

    router.get("/", [](HttpRequest& req, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Welcome to the custom C++ HTTP Server!");
    });

    router.get("/api/users", [](HttpRequest& req, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
        res.setHeader("Content-Type", "application/json");
        res.setBody("{\"users\": [\"Alice\", \"Bob\", \"Charlie\"]}");
    });

    router.post("/api/users", [](HttpRequest& req, HttpResponse& res) {
        if (!req.hasJson()) {
            res.setStatus(HttpStatus::BadRequest);
            res.setHeader("Content-Type", "application/json");
            res.setBody("{\"error\": \"Expected application/json Content-Type\"}");
            return;
        }

        const json& requestJson = req.json();

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
        res.setHeader("Content-Type", "application/json");
        res.setBody(responseJson.dump());
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