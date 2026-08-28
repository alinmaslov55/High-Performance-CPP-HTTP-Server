#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <http/http/HttpHeaders.hpp>

#include <string>
#include <string_view>

namespace http {

enum class HttpStatus{
    OK = 200,
    Created = 201,
    NoContent = 204,

    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,

    InternalServerError = 500,
    NotImplemented = 501,
    ServiceUnavailable = 503

};

class HttpResponse {
public:
    HttpResponse();

    void setStatus(HttpStatus status) noexcept;

    void setHeader(
        std::string name,
        std::string value
    );

    void addHeader(
        std::string name,
        std::string value
    );

    void setBody(std::string body);

    [[nodiscard]]
    HttpStatus status() const noexcept;

    [[nodiscard]]
    std::string_view header(
        std::string_view name
    ) const noexcept;

    [[nodiscard]]
    std::string_view body() const noexcept;

    std::string reasonPhrase(
        const HttpStatus& status
    ) const noexcept;

    [[nodiscard]]
    std::string serialize() const;
private:
    HttpStatus status_;
    HttpHeaders headers_;
    std::string body_;
};

} // namespace http

#endif // HTTP_RESPONSE_HPP