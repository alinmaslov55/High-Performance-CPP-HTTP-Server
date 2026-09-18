#ifndef JWT_UTILS_HPP
#define JWT_UTILS_HPP

#include <string>

namespace http {
namespace utils {

class JwtUtils {
public:
    /**
     * @brief Creates a signed token valid for 24 hours
     */
    static std::string generateToken(const std::string& userId, const std::string& role);
    
    /**
     * @brief Validates the signature and expiration, returning the extracted user ID
     */
    static bool verifyToken(const std::string& token, std::string& outUserId);
};

} // namespace utils
} // namespace http

#endif // JWT_UTILS_HPP