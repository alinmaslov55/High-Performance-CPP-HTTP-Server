#ifndef WEB_SOCKET_UTILS_HPP
#define WEB_SOCKET_UTILS_HPP

#include <string>
#include <string_view>

namespace http {
namespace utils {

class WebSocketUtils {
public:
    /**
     * @brief Generate the Sec-WebSocket-Accept key required for the handshake
     * @param client_key Value of the Sec-WebSocket-Key header from the client
     * @return The base64 encode SHA-1 hash of the key + magic string
     */
    static std::string generateAcceptKey(std::string_view client_key);
};

} // namespace utils
} // namespace http

#endif // WEB_SOCKET_UTILS_HPP