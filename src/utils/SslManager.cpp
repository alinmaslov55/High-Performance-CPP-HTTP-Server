#include "http/utils/SslManager.hpp"

namespace http {
namespace utils {

static int alpn_select_cb(SSL *ssl, const unsigned char **out, unsigned char *outlen,
                          const unsigned char *in, unsigned int inlen, void *arg) {

    // HTTP2 as first option -> fallback to HTTP1.1
    static const unsigned char protos[] = "\x02h2\x08http/1.1";

    int result = SSL_select_next_proto(const_cast<unsigned char **>(out), outlen, protos,
                                       sizeof(protos) - 1, in, inlen);
    if (result != OPENSSL_NPN_NEGOTIATED) {
        return SSL_TLSEXT_ERR_NOACK;
    }
    return SSL_TLSEXT_ERR_OK;
}

void SslManager::initialize(const std::string &cert_path, const std::string &key_path) {
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD *method = TLS_server_method();
    ctx_ = SSL_CTX_new(method);

    if (!ctx_) {
        throw std::runtime_error("Unable to create SSL context");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Configure context to require TLS 1.2 or higher
    SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);

    SSL_CTX_set_alpn_select_cb(ctx_, alpn_select_cb, nullptr);

    // Load the certificate and private key
    if (SSL_CTX_use_certificate_file(ctx_, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load certificate file");
    }
    if (SSL_CTX_use_PrivateKey_file(ctx_, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load private key file");
    }
    if (!SSL_CTX_check_private_key(ctx_)) {
        throw std::runtime_error("Private key does not match the public certificate");
    }
}

} // namespace utils
} // namespace http
