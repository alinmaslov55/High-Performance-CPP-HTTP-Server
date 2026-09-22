#ifndef CRYPTO_UTILS_HPP
#define CRYPTO_UTILS_HPP

#include <string>

namespace http {
namespace utils {

class CryptoUtils {
public:
    /**
     * @brief Hashes a plaintext password using Argon2id (generates a unique salt automatically)
     */
    static std::string hashPassword(const std::string& plaintext);
    
    /**
     * @brief Verifies a plaintext password against an Argon2id hash from the database
     */
    static bool verifyPassword(const std::string& plaintext, const std::string& hash);
};

} // namespace utils
} // namespace http

#endif // CRYPTO_UTILS_HPP