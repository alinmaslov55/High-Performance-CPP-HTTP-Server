#include "http/utils/SslManager.hpp"

namespace http {
namespace utils {

void SslManager::initialize(const std::string& cert_path, const std::string& key_path){
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD* method = TLS_server_method();
    ctx_ = SSL_CTX_new(method);

    if (!ctx_) {
        throw std::runtime_error("Unable to create SSL context");
    }

    // Configure context to require TLS 1.2 or higher
    SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);

    // Load the certificate and private key
    if (SSL_CTX_use_certificate_file(ctx_, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load certificate file");
    }
    if (SSL_CTX_use_PrivateKey_file(ctx_, key_path.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
        throw std::runtime_error("Failed to load private key file");
    }
    if (!SSL_CTX_check_private_key(ctx_)) {
        throw std::runtime_error("Private key does not match the public certificate");
    }
}

} // namespace utis
} // namespace http

