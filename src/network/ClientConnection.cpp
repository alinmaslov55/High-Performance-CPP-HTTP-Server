#include "http/network/TcpServer.hpp"
#include <http/network/ClientConnection.hpp>
#include <http/utils/Logger.hpp>

#include <cerrno>
#include <stdexcept>
#include <utility>

#include <sys/epoll.h>

namespace http {

ClientConnection::ClientConnection(Socket socket, SSL *ssl, const Router &router)
    : socket_(std::move(socket)), ssl_(ssl), handshakeComplete_(false), is_http2_(false),
      h2_session_(nullptr), router_(router) {}

ClientConnection::~ClientConnection() {
    if (h2_session_) {
        nghttp2_session_del(h2_session_);
    }

    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
    }
}

bool ClientConnection::read() {
    char temporaryBuffer[8192];

    while (true) {
        const int bytesReceived = SSL_read(ssl_, temporaryBuffer, sizeof(temporaryBuffer));

        if (bytesReceived > 0) {
            readBuffer_.append(temporaryBuffer, static_cast<std::size_t>(bytesReceived));

            continue;
        }

        int err = SSL_get_error(ssl_, bytesReceived);

        if (err == SSL_ERROR_ZERO_RETURN) {
            return false; // Connection closed gracefully by client
        }

        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return true; // Non-blocking, try again later
        }

        if (err == SSL_ERROR_SYSCALL) {
            return false;
        }

        throw std::runtime_error("SSL_read failed");
    }
}

void ClientConnection::send(const std::string &data) {
    const char *buffer = data.data();
    std::size_t bytesRemaining = data.size();

    while (bytesRemaining > 0) {
        const int bytesSent = SSL_write(ssl_, buffer, bytesRemaining);

        if (bytesSent > 0) {
            buffer += bytesSent;
            bytesRemaining -= static_cast<std::size_t>(bytesSent);

            continue;
        }

        int err = SSL_get_error(ssl_, bytesSent);

        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
            break; // Kernel buffer full, break
        }

        if (err == SSL_ERROR_SYSCALL) {
            return;
        }
        throw std::runtime_error("SSL_write failed");
    }
}

std::string_view ClientConnection::data() const noexcept {
    return readBuffer_.data();
}

ParseResult ClientConnection::parseRequest(HttpRequest &request) {
    return parser_.parse(readBuffer_.data(), request);
}

void ClientConnection::consumeParsedRequest() {
    const std::size_t consumed = parser_.consumedBytes();

    readBuffer_.consume(consumed);

    parser_.reset();
}

void ClientConnection::updateActivity() {
    lastActivity_ = std::chrono::steady_clock::now();
}

bool ClientConnection::isIdle(int timeoutSeconds) const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivity_).count();
    return duration > timeoutSeconds;
}

bool ClientConnection::doHandshake(uint32_t &out_epoll_events) {
    if (handshakeComplete_)
        return true;

    int ret = SSL_accept(ssl_);
    if (ret == 1) {
        handshakeComplete_ = true;

        const unsigned char *alpn_data = nullptr;
        unsigned int alpn_len = 0;
        SSL_get0_alpn_selected(ssl_, &alpn_data, &alpn_len);

        if (alpn_data != nullptr && alpn_len == 2 && alpn_data[0] == 'h' && alpn_data[1] == '2') {
            is_http2_ = true;
            setupHttp2Session();
            LOG_INFO("Negotiated secure HTTP/2 connection");
        }

        return true;
    }

    int err = SSL_get_error(ssl_, ret);
    if (err == SSL_ERROR_WANT_READ) {
        out_epoll_events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        return false;
    } else if (err == SSL_ERROR_WANT_WRITE) {
        out_epoll_events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
        return false;
    }

    throw std::runtime_error("SSL_accept failed");
}

void ClientConnection::upgradeToWebSocket(std::string path) {
    is_websocket_ = true;
    ws_path_ = std::move(path);
    parser_.reset();
}

std::string_view ClientConnection::getWsPath() const {
    return ws_path_;
}

bool ClientConnection::isWebSocket() const {
    return is_websocket_;
}

ParseResult ClientConnection::parseWebSocketFrame(WebSocketFrame &out_frame) {
    std::string_view data = readBuffer_.data();

    // Needed at least 2 bytes just to read the header and length type
    if (data.size() < 2) {
        return ParseResult::Incomplete;
    }

    const auto *bytes = reinterpret_cast<const uint8_t *>(data.data());

    // Decode Byte 0
    bool fin = (bytes[0] & 0x80) != 0;
    uint8_t opcode = bytes[0] & 0x0F;

    // Decode Byte 1
    bool masked = (bytes[1] & 0x80) != 0;
    uint64_t payload_len = bytes[1] & 0x7F;

    size_t header_len = 2;

    // Calculate actual payload length and header size
    if (payload_len == 126) {
        if (data.size() < 4)
            return ParseResult::Incomplete;
        payload_len = (static_cast<uint64_t>(bytes[2]) << 8) | static_cast<uint64_t>(bytes[3]);
        header_len = 4;
    } else if (payload_len == 127) {
        if (data.size() < 10)
            return ParseResult::Incomplete;
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | static_cast<uint64_t>(bytes[2 + i]);
        }
        header_len = 10;
    }

    // Extract the masking key (Client to Server frames MUST be masked)
    uint8_t masking_key[4] = {0};
    if (masked) {
        if (data.size() < header_len + 4)
            return ParseResult::Incomplete;
        for (int i = 0; i < 4; ++i) {
            masking_key[i] = bytes[header_len + i];
        }
        header_len += 4;
    }

    // Ensure we have received the ENTIRE frame over TCP before processing
    if (data.size() < header_len + payload_len) {
        return ParseResult::Incomplete;
    }

    // Extract and unmask the payload (XOR cipher)
    std::string payload;
    payload.resize(payload_len);
    for (size_t i = 0; i < payload_len; ++i) {
        payload[i] = bytes[header_len + i] ^ (masked ? masking_key[i % 4] : 0);
    }

    // Populate the output struct
    out_frame.fin = fin;
    out_frame.opcode = static_cast<WebSocketOpcode>(opcode);
    out_frame.payload = std::move(payload);

    // Consume the exact byte length of this frame from the network buffer
    readBuffer_.consume(header_len + payload_len);

    return ParseResult::Complete;
}

void ClientConnection::sendWebSocketMessage(const std::string &payload, WebSocketOpcode opcode) {
    std::string frame;
    const size_t len = payload.size();

    // Byte 0: FIN flag (0x80) OR'd with the Opcode
    frame.push_back(static_cast<char>(0x80 | static_cast<uint8_t>(opcode)));

    // Byte 1 + Length Extension
    if (len < 126) {
        // For length < 126, Byte 1 is just the length (MASK bit is 0)
        frame.push_back(static_cast<char>(len));
    } else if (len <= 65535) {
        // For length <= 64KB, Byte 1 is 126, followed by 2 bytes of length
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    } else {
        // For massive payloads, Byte 1 is 127, followed by 8 bytes of length
        frame.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<char>((len >> (i * 8)) & 0xFF));
        }
    }

    // Append the raw payload (Server-to-Client frames are NEVER masked)
    frame.append(payload);

    // Send it down the encrypted TLS socket!
    send(frame);
}

void ClientConnection::setupHttp2Session() {
    nghttp2_session_callbacks *callbacks;
    nghttp2_session_callbacks_new(&callbacks);

    nghttp2_session_callbacks_set_send_callback(callbacks, h2_send_cb);
    nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, h2_on_begin_headers_cb);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, h2_on_header_cb);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, h2_on_frame_recv_cb);

    // this == user_data
    nghttp2_session_server_new(&h2_session_, callbacks, this);
    nghttp2_session_callbacks_del(callbacks);

    // HTTP/2 requires the server to send a SETTINGS frame immediately upon connection
    nghttp2_settings_entry iv[1] = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};
    nghttp2_submit_settings(h2_session_, NGHTTP2_FLAG_NONE, iv, 1);
}

ssize_t ClientConnection::h2_send_cb(nghttp2_session *session, const uint8_t *data, size_t length,
                                     int flags, void *user_data) {
    auto *conn = static_cast<ClientConnection *>(user_data);
    try {
        const int bytes_sent = SSL_write(conn->ssl_, data, length);
        if (bytes_sent <= 0) {
            int err = SSL_get_error(conn->ssl_, bytes_sent);
            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                return NGHTTP2_ERR_WOULDBLOCK;
            }
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        return bytes_sent;
    } catch (...) {
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

int ClientConnection::h2_on_begin_headers_cb(nghttp2_session *session, const nghttp2_frame *frame,
                                             void *user_data) {
    auto *conn = static_cast<ClientConnection *>(user_data);
    if (frame->hd.type == NGHTTP2_HEADERS || frame->hd.type == NGHTTP2_PUSH_PROMISE) {
        conn->h2_streams_[frame->hd.stream_id] = HttpRequest();
    }
    return 0;
}

int ClientConnection::h2_on_header_cb(nghttp2_session *session, const nghttp2_frame *frame,
                                      const uint8_t *name, size_t namelen, const uint8_t *value,
                                      size_t valuelen, uint8_t flags, void *user_data) {
    auto *conn = static_cast<ClientConnection *>(user_data);

    std::string_view header_name(reinterpret_cast<const char *>(name), namelen);
    std::string_view header_value(reinterpret_cast<const char *>(value), valuelen);

    if (header_name == ":method") {
        // Map the HTTP/2 string pseudo-header to your HttpMethod enum
        HttpMethod method = HttpMethod::GET; // Default

        if (header_value == "POST")
            method = HttpMethod::POST;
        else if (header_value == "PUT")
            method = HttpMethod::PUT;
        else if (header_value == "DELETE")
            method = HttpMethod::DELETE;
        else if (header_value == "PATCH")
            method = HttpMethod::PATCH;
        else if (header_value == "OPTIONS")
            method = HttpMethod::OPTIONS;
        else if (header_value == "HEAD")
            method = HttpMethod::HEAD;

        conn->h2_streams_[frame->hd.stream_id].setMethod(method);

    } else if (header_name == ":path") {
        conn->h2_streams_[frame->hd.stream_id].setPath(header_value);

    } else {
        conn->h2_streams_[frame->hd.stream_id].setHeader(std::string(header_name),
                                                         std::string(header_value));
    }

    return 0;
}

static ssize_t h2_response_read_cb(nghttp2_session *session, int32_t stream_id, uint8_t *buf,
                                   size_t length, uint32_t *data_flags, nghttp2_data_source *source,
                                   void *user_data) {
    auto *response_body = static_cast<std::string *>(source->ptr);

    if (response_body->empty()) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        delete response_body;
        return 0;
    }

    size_t copy_len = std::min(length, response_body->size());
    std::memcpy(buf, response_body->data(), copy_len);

    response_body->erase(0, copy_len);

    if (response_body->empty()) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        delete response_body;
    }

    return copy_len;
}

bool ClientConnection::isHttp2() const {
    return is_http2_;
}

void ClientConnection::processHttp2() {
    std::string_view data = readBuffer_.data();

    if (!data.empty()) {
        ssize_t rv = nghttp2_session_mem_recv(
            h2_session_, reinterpret_cast<const uint8_t *>(data.data()), data.size());

        if (rv < 0) {
            throw std::runtime_error("nghttp2_session_mem_recv failed");
        }

        readBuffer_.consume(rv);
    }

    nghttp2_session_send(h2_session_);
}

int ClientConnection::h2_on_frame_recv_cb(nghttp2_session *session, const nghttp2_frame *frame,
                                          void *user_data) {
    auto *conn = static_cast<ClientConnection *>(user_data);

    if ((frame->hd.type == NGHTTP2_HEADERS || frame->hd.type == NGHTTP2_DATA) &&
        (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)) {

        int32_t stream_id = frame->hd.stream_id;

        HttpRequest request = std::move(conn->h2_streams_[stream_id]);
        conn->h2_streams_.erase(stream_id);

        TcpServer::total_requests_.fetch_add(1, std::memory_order_relaxed);

        HttpResponse response = conn->router_.handle(request);

        std::vector<nghttp2_nv> h2_headers;

        std::string status_str = std::to_string(static_cast<int>(response.status()));
        h2_headers.push_back({(uint8_t *)":status", (uint8_t *)status_str.c_str(), 7,
                              status_str.length(), NGHTTP2_NV_FLAG_NONE});

        nghttp2_data_provider data_provider;
        data_provider.source.ptr = new std::string(response.body());
        data_provider.read_callback = h2_response_read_cb;

        nghttp2_submit_response(session, stream_id, h2_headers.data(), h2_headers.size(),
                                &data_provider);
    }
    return 0;
}

} // namespace http
