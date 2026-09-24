#include "http/controllers/UserController.hpp"
#include "http/utils/CryptoUtils.hpp"
#include "http/utils/JwtUtils.hpp"
#include "http/middlewares/AuthMiddleware.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <mongocxx/options/find.hpp>

namespace http {
namespace controllers {

using namespace middlewares;

UserController::UserController(repositories::UserRepository & repo) : userRepo_(repo) {}

void UserController::registerRoutes(Router& router) {
    router.post("/api/login", [this](HttpRequest& req, HttpResponse& res){
        this->loginUser(req, res);
    });

    auto requireAuth = middlewares::AuthMiddleware::requireAuth;

    router.post("/api/users", [this, requireAuth](HttpRequest& req, HttpResponse& res) {
        this->createUser(req, res);
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
    if (!extractJson(req, res, payload)) return;

    if(!payload.contains("name") || !payload.contains("password") || !payload["password"].is_string()){
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Missing or invalid 'name' or 'password' field\"}");
        return;
    }

    try {
        std::string name = payload["name"];
        std::string plaintext_password = payload["password"];
        std::string role = payload.contains("role") ? payload["role"] : "user";

        std::string hashed_password = utils::CryptoUtils::hashPassword(plaintext_password);

        if (userRepo_.createUser(name, hashed_password, role)) {
            res.setStatus(HttpStatus::Created);
            res.json("{\"status\": \"success\", \"message\": \"User added!\"}");
        } else {
            res.setStatus(HttpStatus::InternalServerError);
            res.json("{\"error\": \"Failed to create user\"}");
        }
    } catch (const std::exception& e) {
        res.setStatus(HttpStatus::InternalServerError);
        res.json(std::string("{\"error\": \"") + e.what() + "\"}");
    }
}

void UserController::getAllUsers(HttpRequest& req, HttpResponse& res) {
    int limit = 0, skip = 0, sort_order = 1;
    std::string sort_field = std::string(req.query("sort"));

    try {
        if (!req.query("limit").empty()) limit = std::stoi(std::string(req.query("limit")));
        if (!req.query("skip").empty()) skip = std::stoi(std::string(req.query("skip")));
        if (req.query("order") == "desc") sort_order = -1;
    } catch (...) {
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Invalid query parameters\"}");
        return;
    }

    auto users = userRepo_.getAllUsers(limit, skip, sort_field, sort_order);
    
    nlohmann::json response_array = nlohmann::json::array();
    for (const auto& user : users) {
        response_array.push_back(user.toSafeJson());
    }

    res.setStatus(HttpStatus::OK);
    res.json(response_array.dump());
}

void UserController::getUserById(HttpRequest& req, HttpResponse& res) {
    std::string user_id = req.param("id");

    auto user = userRepo_.findById(user_id);
    if (user) {
        res.setStatus(HttpStatus::OK);
        res.json(user->toSafeJson().dump());
    } else {
        res.setStatus(HttpStatus::NotFound);
        res.json("{\"error\": \"User not found or invalid ID\"}");
    }
}

void UserController::updateUser(HttpRequest& req, HttpResponse& res) {
    std::string user_id = req.param("id");
    nlohmann::json payload;

    if (!extractJson(req, res, payload)) return;

    // Prevent updating sensitive fields manually
    payload.erase("_id");
    payload.erase("password");
    payload.erase("password_hash");

    if (payload.empty()) {
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"No valid fields to update\"}");
        return;
    }

    if (userRepo_.updateUser(user_id, payload)) {
        res.setStatus(HttpStatus::OK);
        res.json("{\"status\": \"success\", \"message\": \"User updated\"}");
    } else {
        res.setStatus(HttpStatus::NotFound);
        res.json("{\"error\": \"User not found or invalid ID\"}");
    }
}

void UserController::deleteUser(HttpRequest& req, HttpResponse& res) {
    std::string user_id = req.param("id");

    if (userRepo_.deleteUser(user_id)) {
        res.setStatus(HttpStatus::OK);
        res.json("{\"status\": \"success\", \"message\": \"User deleted\"}");
    } else {
        res.setStatus(HttpStatus::NotFound);
        res.json("{\"error\": \"User not found or invalid ID\"}");
    }
}

void UserController::loginUser(HttpRequest& req, HttpResponse& res){
    nlohmann::json payload;

    if (!extractJson(req, res, payload)) return;

    if (!payload.contains("name") || !payload.contains("password")) {
        res.setStatus(HttpStatus::BadRequest);
        res.json("{\"error\": \"Missing 'name' or 'password'\"}");
        return;
    }

    std::string username = payload["name"];
    std::string plaintext_password = payload["password"];

    auto user = userRepo_.findByUsername(username);

    if(!user || user->password_hash.empty()){
        res.setStatus(HttpStatus::Unauthorized);
        res.json("{\"error\": \"Invalid username or password\"}");
        return;
    }

    if (!utils::CryptoUtils::verifyPassword(plaintext_password, user->password_hash)) {
        res.setStatus(HttpStatus::Unauthorized);
        res.json("{\"error\": \"Invalid username or password\"}");
        return;
    }

    std::string token = utils::JwtUtils::generateToken(user->id, user->role);

    res.setStatus(HttpStatus::OK);
    res.json("{\"status\": \"success\", \"token\": \"" + token + "\"}");
}

bool UserController::extractJson(HttpRequest& req, HttpResponse& res, nlohmann::json& out_payload) {
    if (req.hasJson()) {
        out_payload = req.json();
        return true;
    } else if (!req.body().empty()) {
        try {
            out_payload = nlohmann::json::parse(req.body());
            return true;
        } catch (...) {
            res.setStatus(HttpStatus::BadRequest);
            res.json("{\"error\": \"Invalid JSON\"}");
            return false;
        }
    }
    res.setStatus(HttpStatus::BadRequest);
    res.json("{\"error\": \"Missing request body\"}");
    return false;
}

} // namespace controllers
} // namespace http
