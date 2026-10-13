#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <http/http/HttpRequest.hpp>

namespace http {

enum class ParseResult { Complete, Incomplete, Invalid };

class HttpParser {
public:
    static constexpr std::size_t MAX_REQUEST_LINE = 8192;
    static constexpr std::size_t MAX_HEADER_SIZE = 16384;
    static constexpr std::size_t MAX_HEADERS = 100;
    static constexpr std::size_t MAX_BODY_SIZE = 10 * 1024 * 1024;
    static constexpr std::size_t MAX_CHUNK_LINE_SIZE = 128;

    ParseResult parse(std::string_view data, HttpRequest &request);

    [[nodiscard]]
    std::size_t consumedBytes() const noexcept;

    void reset() noexcept;

private:
    enum class State {
        RequestLine,
        Headers,
        Body,
        ChunkSize,
        ChunkData,
        ChunkDataCrlf,
        ChunkTrailer,
        Complete,
        Error
    };

    std::optional<ParseResult> handleRequestLine(std::string_view data, HttpRequest &request);
    std::optional<ParseResult> handleHeaders(std::string_view data, HttpRequest &request);
    std::optional<ParseResult> handleBody(std::string_view data, HttpRequest &request);
    std::optional<ParseResult> handleChunkSize(std::string_view data);
    std::optional<ParseResult> handleChunkData(std::string_view data);
    std::optional<ParseResult> handleChunkDataCrlf(std::string_view data);
    std::optional<ParseResult> handleChunkTrailer(std::string_view data, HttpRequest &request);

    ParseResult fail() noexcept;

    ParseResult parseRequestLine(std::string_view data, HttpRequest &request) const;
    ParseResult parseHeaders(std::string_view data, HttpRequest &request) const;
    ParseResult parseBody(std::string_view data, HttpRequest &request) const;

    State state_ = State::RequestLine;
    std::size_t consumed_ = 0;
    std::size_t bodySize_ = 0;
    std::size_t chunkSize_ = 0;
    std::size_t chunkBytesRead_ = 0;

    std::string body_;
};

} // namespace http

#endif // HTTP_PARSER_HPP