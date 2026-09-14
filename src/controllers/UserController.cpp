#include "http/controllers/UserController.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

namespace http {
namespace controllers {

UserController::UserController(db::MongoPool& db_pool) : db_pool_(db_pool) {}

void UserController::registerRoutes(Router& router) {
    router.post("/api/users", [this](HttpRequest& req, HttpResponse& res) {
        this->createUser(req, res);
    });

    router.get("/api/users", [this](HttpRequest& req, HttpResponse& res) {
        this->getAllUsers(req, res);
    });
}

void UserController::createUser(HttpRequest& req, HttpResponse& res) {
    if (!req.hasJson()) {
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Invalid JSON\"}");
        return;
    }

    try {
        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];

        bsoncxx::document::value document = bsoncxx::from_json(req.json().dump());
        collection.insert_one(document.view());

        res.setStatus(HttpStatus::Created);
        res.json("{\"status\": \"success\", \"message\": \"User added!\"}");

    } catch (const std::exception& e) {
        res.setStatus(HttpStatus::InternalServerError);
        res.json(std::string("{\"error\": \"") + e.what() + "\"}");
    }
}

void UserController::getAllUsers(HttpRequest& req, HttpResponse& res) {
    try {
        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];

        auto cursor = collection.find({});
        nlohmann::json response_array = nlohmann::json::array();

        for (auto&& doc : cursor) {
            response_array.push_back(nlohmann::json::parse(bsoncxx::to_json(doc)));
        }

        res.setStatus(HttpStatus::OK);
        res.json(response_array.dump());

    } catch (const std::exception& e) {
        res.setStatus(HttpStatus::InternalServerError);
        res.json(std::string("{\"error\": \"") + e.what() + "\"}");
    }
}

} // namespace controllers
} // namespace http