#include "http/controllers/UserController.hpp"
#include "http/utils/JwtUtils.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <mongocxx/options/find.hpp>

namespace http {
namespace controllers {

UserController::UserController(db::MongoPool& db_pool) : db_pool_(db_pool) {}

void UserController::registerRoutes(Router& router) {
    router.post("/api/login", [this](HttpRequest& req, HttpResponse& res){
        this->loginUser(req, res);
    });

    auto requireAuth = [](HttpRequest& req, HttpResponse& res) -> bool {
        std::string auth_header = std::string(req.header("Authorization")); 
        
        if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
            res.setStatus(HttpStatus::Unauthorized);
            res.json("{\"error\": \"Missing or invalid Authorization header\"}");
            return false;
        }

        std::string token = auth_header.substr(7);
        std::string user_id;
        
        if (!utils::JwtUtils::verifyToken(token, user_id)) {
            res.setStatus(HttpStatus::Unauthorized);
            res.json("{\"error\": \"Invalid or expired token\"}");
            return false;
        }
        
        return true; // Valid Token
    };

    router.post("/api/users", [this, requireAuth](HttpRequest& req, HttpResponse& res) {
        if (requireAuth(req, res)) this->createUser(req, res);
    });

    router.get("/api/users", [this, requireAuth](HttpRequest& req, HttpResponse& res) {
        if (requireAuth(req, res)) this->getAllUsers(req, res);
    });

    router.get("/api/users/:id", [this, requireAuth](HttpRequest& req, HttpResponse& res) {
        if (requireAuth(req, res)) this->getUserById(req, res);
    });

    router.put("/api/users/:id", [this, requireAuth](HttpRequest& req, HttpResponse& res) {
        if (requireAuth(req, res)) this->updateUser(req, res);
    });

    router.del("/api/users/:id", [this, requireAuth](HttpRequest& req, HttpResponse& res) {
        if (requireAuth(req, res)) this->deleteUser(req, res);
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

        mongocxx::options::find opts;

        auto limit_str = req.query("limit");
        if(!limit_str.empty()){
            try {
                opts.limit(std::stoi(std::string(limit_str)));
            } catch (...){
                res.setStatus(HttpStatus::BadRequest);
                res.json("{\"error\": \"Invalid limit parameter\"}");
                return;
            }
        }

        auto skip_str = req.query("skip");
        if (!skip_str.empty()) {
            try {
                opts.skip(std::stoi(std::string(skip_str)));
            } catch (...) {
                res.setStatus(HttpStatus::BadRequest);
                res.json("{\"error\": \"Invalid skip parameter\"}");
                return;
            }
        }

        auto sort_field = req.query("sort");
        if(!sort_field.empty()){
            int sort_direction = (req.query("order") == "desc")? -1 : 1;

            opts.sort(bsoncxx::builder::stream::document{}
                << std::string(sort_field) << sort_direction << bsoncxx::builder::stream::finalize
            );
        }

        auto cursor = collection.find({}, opts);
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

void UserController::updateUser(HttpRequest& req, HttpResponse& res) {
    std::string user_id = req.param("id");
    nlohmann::json payload;

    if(req.hasJson()){
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

    payload.erase("_id");

    if(payload.empty()){
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"No fields to update\"}");
        return;
    }

    try {
        bsoncxx::oid document_id(user_id);

        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];
        
        auto filter = bsoncxx::builder::stream::document{} 
            << "_id" << document_id
            << bsoncxx::builder::stream::finalize;

        bsoncxx::document::value update_doc = bsoncxx::from_json(payload.dump());

        auto update = bsoncxx::builder::stream::document{}
            << "$set" << update_doc.view()
            << bsoncxx::builder::stream::finalize;
        
        auto result = collection.update_one(filter.view(), update.view());

        if(result && result->matched_count() > 0){
            res.setStatus(HttpStatus::OK);
            res.json("{\"status\": \"success\", \"message\": \"User updated\"}");
        } else {
            res.setStatus(HttpStatus::NotFound);
            res.json("{\"error\": \"User not found\"}");
        }
    } catch (const bsoncxx::exception&){
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Invalid user ID format\"}");
    } catch (const std::exception& e){
        res.setStatus(HttpStatus::InternalServerError);
        res.json(std::string("{\"error\": \"") + e.what() + "\"}");
    }
}

void UserController::deleteUser(HttpRequest& req, HttpResponse& res){
    std::string user_id = req.param("id");

    try {
        bsoncxx::oid document_id(user_id);

        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];

        auto filter = bsoncxx::builder::stream::document{}
            << "_id" << document_id
            << bsoncxx::builder::stream::finalize;
        
        auto result = collection.delete_one(filter.view());

        if(result && result->deleted_count() > 0){
            res.setStatus(HttpStatus::OK);
            res.json("{\"status\": \"success\", \"message\": \"User deleted\"}");
        } else {
            res.setStatus(HttpStatus::NotFound);
            res.json("{\"error\": \"User not found\"}");
        }
    } catch (const bsoncxx::exception&){
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Invalid user ID format\"}");
    } catch (const std::exception& e){
        res.setStatus(HttpStatus::InternalServerError);
        res.json(std::string("{\"error\": \"") + e.what() + "\"}");
    }
}

void UserController::loginUser(HttpRequest& req, HttpResponse& res){
    nlohmann::json payload;

    try {
        payload = req.hasJson()? req.json(): nlohmann::json::parse(req.body());
    } catch (...){
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Invalid JSON\"}");
        return;
    }

    if (!payload.contains("name")) {
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Missing 'name' for login\"}");
        return;
    }

    try {
        std::string username = payload["name"];

        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];
        auto query = bsoncxx::builder::stream::document{}
            << "name" << username
            << bsoncxx::builder::stream::finalize;

        auto result = collection.find_one(query.view());

        if(result){
            auto view = result->view();
            std::string id_str = view["_id"].get_oid().value.to_string();

            std::string role = "user";

            if(view["role"]){
                role = std::string(view["role"].get_string().value);
            }

            std::string token = utils::JwtUtils::generateToken(id_str, role);

            res.setStatus(HttpStatus::OK);
            nlohmann::json response;
            response["status"] = "success";
            response["token"] = token;
            res.json(response.dump());
        } else {
            res.setStatus(HttpStatus::Unauthorized);
            res.json("{\"error\": \"User not found\"}");
        }
    } catch (std::exception& e){
        res.setStatus(HttpStatus::InternalServerError);
        res.json(std::string("{\"error\": \"") + e.what() + "\"}");
    }
}

} // namespace controllers
} // namespace http
