#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include <string>

#include <http/http/HttpRequest.hpp>

namespace http {

enum class ParseResult {
    Complete,
    Incomplete,
    Invalid
};

class HttpParser{
public:
    static constexpr std::size_t MAX_REQUEST_LINE = 8192;
    static constexpr std::size_t MAX_HEADER_SIZE = 16384;
    static constexpr std::size_t MAX_HEADERS = 100;

    ParseResult parse(std::string_view data, HttpRequest& request);
    void reset() noexcept;
private:
    enum class State {
        RequestLine,
        Headers,
        Body,
        Complete,
        Error
    };
    
    ParseResult parseHeaders(std::string_view data, HttpRequest& request) const;
    ParseResult parseRequestLine(std::string_view data, HttpRequest& request) const;
    ParseResult parseBody(std::string_view data, HttpRequest& request) const;

    State state_ = State::RequestLine;
    std::size_t consumed_ = 0;
};

} // namespace http

#endif // HTTP_PARSER_HPP 