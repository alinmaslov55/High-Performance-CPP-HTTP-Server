#include <gtest/gtest.h>
#include "http/controllers/UserController.hpp"
#include "http/repositories/UserRepository.hpp"
#include "http/http/Router.hpp"
#include "http/utils/JwtUtils.hpp"
#include <mongocxx/instance.hpp>

namespace http_tests {

using namespace http;
using namespace http::controllers;

static mongocxx::instance db_instance{};

class UserControllerTest : public ::testing::Test {
protected:
    db::MongoPool* db_pool;
    repositories::UserRepository* user_repo;
    UserController* controller;
    Router router;

    void SetUp() override {
        db_pool = new db::MongoPool("mongodb://localhost:27017");
        user_repo = new repositories::UserRepository(*db_pool);
        controller = new UserController(*user_repo);
        
        controller->registerRoutes(router);
    }

    void TearDown() override {
        delete controller;
        delete user_repo;
        delete db_pool;
    }

    HttpRequest createRequest(HttpMethod method, std::string_view path, const std::string& body = "") {
        HttpRequest req;
        req.setMethod(method);
        req.setPath(path);
        if (!body.empty()) {
            req.setBody(body);
        }
        return req;
    }
};

TEST_F(UserControllerTest, MiddlewareRejectsMissingToken) {
    auto req = createRequest(HttpMethod::GET, "/api/users");
    auto res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::Unauthorized);
    EXPECT_TRUE(res.body().find("Missing or invalid Authorization header") != std::string::npos);
}

TEST_F(UserControllerTest, MiddlewareRejectsGarbageToken) {
    auto req = createRequest(HttpMethod::GET, "/api/users");
    req.setHeader("Authorization", "Bearer some.garbage.token");
    auto res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::Unauthorized);
    EXPECT_TRUE(res.body().find("Invalid or expired token") != std::string::npos);
}

TEST_F(UserControllerTest, LoginRejectsMalformedJson) {
    auto req = createRequest(HttpMethod::POST, "/api/login", "{ bad_json: ");
    auto res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::BadRequest);
    EXPECT_TRUE(res.body().find("Invalid JSON") != std::string::npos);
}

TEST_F(UserControllerTest, LoginRejectsMissingName) {
    auto req = createRequest(HttpMethod::POST, "/api/login", "{\"email\": \"john@example.com\"}");
    auto res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::BadRequest);
    EXPECT_TRUE(res.body().find("Missing 'name' or 'password'") != std::string::npos);
}

TEST_F(UserControllerTest, UpdateRejectsEmptyPayloadAfterIdErased) {
    std::string token = utils::JwtUtils::generateToken("test_id", "admin");
    
    auto req = createRequest(HttpMethod::PUT, "/api/users/123", "{\"_id\": \"123\"}");
    req.setHeader("Authorization", "Bearer " + token);
    
    auto res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::BadRequest);
    EXPECT_TRUE(res.body().find("No valid fields to update") != std::string::npos);
}

// A valid token is needed to pass the auth middleware before the payload is validated
TEST_F(UserControllerTest, CreateUserRejectsMissingPassword) {
    std::string token = utils::JwtUtils::generateToken("test_id", "admin");
    
    auto req = createRequest(HttpMethod::POST, "/api/users", "{\"name\": \"Bob\"}");
    req.setHeader("Authorization", "Bearer " + token);
    
    auto res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::BadRequest);
    EXPECT_TRUE(res.body().find("Missing or invalid 'name' or 'password' field") != std::string::npos);
}

TEST_F(UserControllerTest, LoginRejectsMissingPassword) {
    auto req = createRequest(HttpMethod::POST, "/api/login", "{\"name\": \"Bob\"}");
    auto res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::BadRequest);
    EXPECT_TRUE(res.body().find("Missing 'name' or 'password'") != std::string::npos);
}

} // namespace http_tests