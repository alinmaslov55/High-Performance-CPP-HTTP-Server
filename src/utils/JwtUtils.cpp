#include "http/utils/JwtUtils.hpp"
#include "http/utils/Logger.hpp"
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <cstdlib>
#include <chrono>

namespace http {
namespace utils {

using jwt_traits = jwt::traits::nlohmann_json;
using jwt_claim  = jwt::basic_claim<jwt_traits>;

static std::string getSecret() {
    const char* secret = std::getenv("JWT_SECRET");
    return secret ? std::string(secret) : "default_unsafe_secret";
}

std::string JwtUtils::generateToken(const std::string& userId, const std::string& role) {
    auto now = std::chrono::system_clock::now();
    
    return jwt::create<jwt_traits>()
        .set_issuer("high_performance_cpp_server")
        .set_type("JWS")
        .set_payload_claim("id", jwt_claim(userId))
        .set_payload_claim("role", jwt_claim(role))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(24))
        .sign(jwt::algorithm::hs256{getSecret()});
}

bool JwtUtils::verifyToken(const std::string& token, std::string& outUserId) {
    try {
        // Explicitly use the nlohmann traits here
        auto decoded = jwt::decode<jwt_traits>(token);
        
        auto verifier = jwt::verify<jwt_traits>()
            .allow_algorithm(jwt::algorithm::hs256{getSecret()})
            .with_issuer("high_performance_cpp_server");
            
        verifier.verify(decoded);
        
        outUserId = decoded.get_payload_claim("id").as_string();
        return true;
    } catch (const std::exception& e) {
        LOG_WARN("JWT Verification failed: {}", e.what());
        return false;
    }
}

} // namespace utils
} // namespace http