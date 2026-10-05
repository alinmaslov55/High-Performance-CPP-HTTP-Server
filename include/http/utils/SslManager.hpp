#ifndef SSL_MANAGER_HPP
#define SSL_MANAGER_HPP

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <mutex>
#include <stdexcept>
#include <string>

namespace http {
namespace utils {

class SslManager {
public:
    static SslManager &getInstance() {
        static SslManager instance;
        return instance;
    }

    void initialize(const std::string &cert_path, const std::string &key_path);
    SSL_CTX *getContext() {
        return ctx_;
    }
    ~SslManager() {
        if (ctx_) {
            SSL_CTX_free(ctx_);
        }
        EVP_cleanup();
    }

private:
    SslManager() : ctx_(nullptr) {}
    SSL_CTX *ctx_;
    std::mutex mutex_;
};

} // namespace utils
} // namespace http

#endif // SSL_MANAGER