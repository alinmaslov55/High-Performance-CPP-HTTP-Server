#ifndef RATE_LIMITER_HPP
#define RATE_LIMITER_HPP

#include "http/http/HttpRequest.hpp"
#include "http/http/HttpResponse.hpp"
#include <sw/redis++/redis++.h>
#include <memory>
#include <string>
#include <functional>

namespace http {
namespace middlewares {

class RateLimiter {
public:
    static std::function<bool(HttpRequest&, HttpResponse&)> create(
        std::shared_ptr<sw::redis::Redis> redis_client,
        int max_requests,
        int window_seconds
    );
};

} // namespace middlewares
} // namespace http

#endif // RATE_LIMITER_HPP