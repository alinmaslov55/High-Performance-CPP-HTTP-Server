#include "http/utils/CryptoUtils.hpp"
#include "http/utils/Logger.hpp"
#include <sodium.h>
#include <stdexcept>
#include <mutex>

namespace http {
namespace utils {

static std::once_flag sodium_init_flag;

static void ensureSodiumInitialized() {
    std::call_once(sodium_init_flag, []() {
        if (sodium_init() < 0) {
            LOG_ERROR("Failed to initialize libsodium!");
            throw std::runtime_error("libsodium initialization failed");
        }
    });
}

std::string CryptoUtils::hashPassword(const std::string& plaintext) {
    ensureSodiumInitialized();

    char hash[crypto_pwhash_STRBYTES];
    
    if (crypto_pwhash_str(
            hash, 
            plaintext.c_str(), 
            plaintext.length(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("Out of memory while hashing password");
    }

    return std::string(hash);
}

bool CryptoUtils::verifyPassword(const std::string& plaintext, const std::string& hash) {
    ensureSodiumInitialized();

    return crypto_pwhash_str_verify(
        hash.c_str(), 
        plaintext.c_str(), 
        plaintext.length()
    ) == 0;
}

} // namespace utils
} // namespace http