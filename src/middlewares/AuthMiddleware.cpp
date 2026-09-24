#include "http/middlewares/AuthMiddleware.hpp"

namespace http {
namespace middlewares {

bool AuthMiddleware::requireAuth(HttpRequest& req, HttpResponse& res) {
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
    
    // Optional
    // req.setAttribute("user_id", user_id); 
    return true;
}

} // namespace middlewares
} // namespace http