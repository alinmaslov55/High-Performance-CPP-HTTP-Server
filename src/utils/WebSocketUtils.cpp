#include "http/utils/WebSocketUtils.hpp"

#include <vector>

#include <openssl/evp.h>
#include <openssl/sha.h>

namespace http {
namespace utils {

std::string WebSocketUtils::generateAcceptKey(std::string_view client_key) {
    // The RFC 6455 Magic String
    static const std::string WS_MAGIC_STRING = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    // Concatenate client key and magic string
    std::string combined(client_key);
    combined += WS_MAGIC_STRING;

    // Calculate SHA-1 hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(combined.c_str()), combined.length(), hash);

    // Base64 Encode the hash using OpenSSL's EVP API
    // EVP_EncodeBlock adds a null terminator, needed +1 for the buffer
    std::vector<unsigned char> base64_buffer(((SHA_DIGEST_LENGTH + 2) / 3) * 4 + 1);
    int encoded_len = EVP_EncodeBlock(base64_buffer.data(), hash, SHA_DIGEST_LENGTH);

    return std::string(reinterpret_cast<char *>(base64_buffer.data()), encoded_len);
}

} // namespace utils
} // namespace http