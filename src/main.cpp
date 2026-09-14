#include "http/network/TcpServer.hpp"
#include "http/http/Router.hpp"
#include "http/database/MongoPool.hpp"
#include <nlohmann/json.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <iostream>
#include <csignal>

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
    ::signal(SIGPIPE, SIG_IGN);

    mongocxx::instance instance{};
    db::MongoPool db_pool("mongodb://localhost:27017");

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

    router.post("/api/users", [&db_pool](HttpRequest& req, HttpResponse& res) {
        if (!req.hasJson()) {
            res.setStatus(HttpStatus::BadRequest);
            res.json("{\"error\": \"Invalid JSON\"}");
            return;
        }

        try {
            auto conn = db_pool.acquire();
            auto collection = (*conn)["test_db"]["users"];

            bsoncxx::document::value document = bsoncxx::from_json(req.json().dump());
            collection.insert_one(document.view());

            res.setStatus(HttpStatus::Created);
            res.json("{\"status\": \"success\", \"message\": \"User added!\"}");

        } catch (const std::exception& e) {
            res.setStatus(HttpStatus::InternalServerError);
            res.json(std::string("{\"error\": \"") + e.what() + "\"}");
        }
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
