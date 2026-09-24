#ifndef AUTH_MIDDLEWARES_HPP
#define AUTH_MIDDLEWARES_HPP

#include "http/http/HttpRequest.hpp"
#include "http/http/HttpResponse.hpp"
#include "http/utils/JwtUtils.hpp"

namespace http {
namespace middlewares {

class AuthMiddleware {
public:
    static bool requireAuth(HttpRequest& req, HttpResponse& res);
};

} // namespace middlewares
} // namespace http

#endif // AUTH_MIDDLEWARES_HPP