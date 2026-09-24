#ifndef USER_CONTROLLER_HPP
#define USER_CONTROLLER_HPP

#include "http/http/Router.hpp"
#include "http/repositories/UserRepository.hpp"

namespace http {

namespace controllers {
class UserController {
public:
    explicit UserController(repositories::UserRepository & repo);

    void registerRoutes(Router& router);

private:
    void createUser(HttpRequest& req, HttpResponse& res);
    void getAllUsers(HttpRequest& req, HttpResponse& res);
    void getUserById(HttpRequest& req, HttpResponse& res);
    void updateUser(HttpRequest& req, HttpResponse& res);
    void deleteUser(HttpRequest& req, HttpResponse& res);
    void loginUser(HttpRequest& req, HttpResponse& res);

    bool extractJson(HttpRequest& req, HttpResponse& res, nlohmann::json& out_payload);

    repositories::UserRepository& userRepo_;
};
} // namespace controllers
} // namespace http

#endif // USER_CONTROLLER_HPP
