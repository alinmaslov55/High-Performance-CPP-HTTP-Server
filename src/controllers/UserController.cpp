#include "http/controllers/UserController.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/exception/exception.hpp>


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

    router.get("/api/users/:id", [this](HttpRequest& req, HttpResponse& res) {
        this->getUserById(req, res);
    });
}

void UserController::createUser(HttpRequest& req, HttpResponse& res) {
    nlohmann::json payload;

    if (req.hasJson()) {
        payload = req.json();
    } else if (!req.body().empty()) {
        try {
            payload = nlohmann::json::parse(req.body());
        } catch (...) {
            res.setStatus(HttpStatus::BadRequest);
            res.json("{\"error\": \"Invalid JSON\"}");
            return;
        }
    } else {
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Missing request body\"}");
        return;
    }

    try {
        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];

        bsoncxx::document::value document = bsoncxx::from_json(payload.dump());
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

void UserController::getUserById(HttpRequest& req, HttpResponse& res) {
    std::string user_id = req.param("id");

    try {
        bsoncxx::oid document_id(user_id);

        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];
        auto query = bsoncxx::builder::stream::document{}
            << "_id" << document_id
            << bsoncxx::builder::stream::finalize;
        auto result = collection.find_one(query.view());

        if(result){
            res.setStatus(HttpStatus::OK);
            res.json(bsoncxx::to_json(*result));
        } else {
            res.setStatus(HttpStatus::NotFound);
            res.json(std::string("{\"error\": \"User not found\"}"));
        }
    } catch (const bsoncxx::exception& e) {
        res.setStatus(HttpStatus::BadRequest);
        res.json(std::string("{\"error\": \"Invalid user ID format\"}"));
    } catch (const std::exception& e) {
        res.setStatus(HttpStatus::InternalServerError);
        res.json(std::string("{\"error\": \"") + e.what() + "\"}");
    }
}
} // namespace controllers
} // namespace http
