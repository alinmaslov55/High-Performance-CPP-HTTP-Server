#include "http/middlewares/RateLimiter.hpp"

namespace http {
namespace middlewares {

std::function<bool(HttpRequest&, HttpResponse&)> RateLimiter::create(
    std::shared_ptr<sw::redis::Redis> redis_client,
    int max_requests,
    int window_seconds
){
    return [redis_client, max_requests, window_seconds](HttpRequest& req, HttpResponse& res) -> bool {
        std::string client_id = std::string(req.header("X-Forwarded-For"));
        if (client_id.empty()) {
            client_id = "unknown_ip"; 
        }
        std::string redis_key = "rate_limit:" + client_id;

        try {
            long long current_count = redis_client->incr(redis_key);
            if (current_count == 1) {
                redis_client->expire(redis_key, window_seconds);
            }

            if (current_count > max_requests) {
                res.setStatus(HttpStatus::TooManyRequests); // HTTP 429
                res.json("{\"error\": \"Too many requests. Please slow down.\"}");
                return false;
            }

            return true;
        } catch (const sw::redis::Error& e){
            return true;
        }
    };
}

} // namespace middlewares
} // namespace http
