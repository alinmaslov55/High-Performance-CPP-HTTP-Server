#ifndef HTTP_UTILS_HPP
#define HTTP_UTILS_HPP

#include <http/http/HttpParser.hpp>

#include <charconv>
#include <limits>

namespace http {
namespace utils {

HttpMethod parseMethod(std::string_view method);
std::string_view trimOws(std::string_view value);
bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs);
bool isValidHeaderName(std::string_view name);
bool isValidHeaderValue(std::string_view value);
bool parseContentLength(std::string_view value, std::size_t &result);
bool parseChunkSize(std::string_view value, std::size_t &result);
std::string urlDecode(std::string_view value);

} // namespace utils
} // namespace http

#endif // HTTP_UTILS_HPP