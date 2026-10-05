#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

#include <nghttp2/nghttp2.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "http/http/HttpParser.hpp"
#include "http/http/HttpRequest.hpp"
#include "http/http/Router.hpp"
#include "http/network/ReadBuffer.hpp"
#include "http/network/Socket.hpp"

namespace http {

enum class WebSocketOpcode : uint8_t {
    Continuation = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xA
};

struct WebSocketFrame {
    bool fin;
    WebSocketOpcode opcode;
    std::string payload;
};

/**
 * @brief Active session with a client
 * @todo Arrange local variables in different order for memory optimisation across all
 * files(classes)
 */
class ClientConnection {
public:
    explicit ClientConnection(Socket socket, SSL *ssl, const Router &router);

    ClientConnection(const ClientConnection &) = delete;
    ClientConnection &operator=(const ClientConnection &) = delete;

    ClientConnection(ClientConnection &&) noexcept = delete;
    ClientConnection &operator=(ClientConnection &&) noexcept = delete;

    ~ClientConnection();

    bool doHandshake(uint32_t &out_epoll_events);
    bool isHandshakeComplete() const {
        return handshakeComplete_;
    }

    /**
     * @brief Reads data from socket and appends to an internal buffer
     * @throws std::runtime_error on read error
     * @return true if data was read successfully, flas eif connection was
     * closed by peer
     */
    [[nodiscard]]
    bool read();

    /**
     * @brief Sends a string over the socket
     * @param data payload to be sent
     * @throws std::runtime_error
     */
    void send(const std::string &data);

    /**
     * @brief Retrieves a read-only wrapper over the data from ReadBuffer
     * @return std::string_view with the raw bytes
     */
    [[nodiscard]]
    std::string_view data() const noexcept;

    // HTTP Parsing
    ParseResult parseRequest(HttpRequest &request);
    void consumeParsedRequest();

    // WebSocket State and Parsing
    void upgradeToWebSocket(std::string path);
    [[nodiscard]]
    std::string_view getWsPath() const;
    [[nodiscard]]
    bool isWebSocket() const;
    ParseResult parseWebSocketFrame(WebSocketFrame &out_frame);
    void sendWebSocketMessage(const std::string &payload,
                              WebSocketOpcode opcode = WebSocketOpcode::Text);

    void updateActivity();

    [[nodiscard]]
    bool isIdle(int timeoutSeconds) const;

    [[nodiscard]]
    bool isHttp2() const;

    /**
     * @brief Feeds the network buffer into nghttp2 and triggers outbound frames
     */
    void processHttp2();

private:
    void setupHttp2Session();

    static ssize_t h2_send_cb(nghttp2_session *session, const uint8_t *data, size_t length,
                              int flags, void *user_data);
    static int h2_on_begin_headers_cb(nghttp2_session *session, const nghttp2_frame *frame,
                                      void *user_data);
    static int h2_on_header_cb(nghttp2_session *session, const nghttp2_frame *frame,
                               const uint8_t *name, size_t namelen, const uint8_t *value,
                               size_t valuelen, uint8_t flags, void *user_data);
    static int h2_on_frame_recv_cb(nghttp2_session *session, const nghttp2_frame *frame,
                                   void *user_data);

    Socket socket_;
    SSL *ssl_;
    bool handshakeComplete_;
    bool is_websocket_{false};
    std::string ws_path_;
    bool is_http2_{false};
    nghttp2_session *h2_session_{nullptr};
    std::unordered_map<uint32_t, HttpRequest> h2_streams_;
    const Router &router_;

    ReadBuffer readBuffer_;
    HttpParser parser_;
    std::chrono::steady_clock::time_point lastActivity_ = std::chrono::steady_clock::now();
};

} // namespace http

#endif // CLIENT_CONNECTION_HPP
