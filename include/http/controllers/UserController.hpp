#ifndef USER_CONTROLLER_HPP
#define USER_CONTROLLER_HPP

#include "http/http/Router.hpp"
#include "http/database/MongoPool.hpp"

namespace http {

namespace controllers {
class UserController {
public:
    explicit UserController(db::MongoPool& db_pool);

    void registerRoutes(Router& router);

private:
    void createUser(HttpRequest& req, HttpResponse& res);
    void getAllUsers(HttpRequest& req, HttpResponse& res);
    void getUserById(HttpRequest& req, HttpResponse& res);
    void updateUser(HttpRequest& req, HttpResponse& res);
    void deleteUser(HttpRequest& req, HttpResponse& res);

    void loginUser(HttpRequest& req, HttpResponse& res);

    db::MongoPool& db_pool_;
};
} // namespace controllers
} // namespace http

#endif // USER_CONTROLLER_HPP
