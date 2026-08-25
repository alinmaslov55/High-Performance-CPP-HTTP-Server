#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <string_view>
#include <unordered_map>

namespace http {

enum class HttpMethod{
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
    HEAD,
    OPTIONS,
    CONNECT,
    TRACE,
    UNKNOWN
};

class HttpRequest{
public:
    HttpRequest() = default;
    HttpRequest(HttpMethod method, std::string target, std::string version);

    HttpRequest(const HttpRequest&) = default;
    HttpRequest& operator=(const HttpRequest&) = default;

    HttpRequest(HttpRequest&&) noexcept = default;
    HttpRequest& operator=(HttpRequest&&) noexcept = default;
    
    [[nodiscard]]
    HttpMethod method() const noexcept;

    [[nodiscard]]
    std::string_view target() const noexcept;

    [[nodiscard]]
    std::string_view version() const noexcept;

    [[nodiscard]]
    std::string_view header(std::string_view name) const noexcept;

    [[nodiscard]]
    bool hasHeader(std::string_view name) const noexcept;

    [[nodiscard]]
    std::string_view body() const noexcept;

    void setHeader(std::string name, std::string value);
    void setBody(std::string body);

private:
    static std::string normalizeHeaderName(std::string_view name);

    HttpMethod method_;
    std::string target_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

} // namespace http

#endif // HTTP_REQUEST_HPP