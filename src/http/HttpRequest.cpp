#include <http/http/HttpRequest.hpp>

#include <utility>

namespace http {

HttpRequest::HttpRequest(HttpMethod method, std::string target,
						 std::string version)
	: method_(method), target_(std::move(target)),
	  version_(std::move(version)) {}

HttpMethod HttpRequest::method() const noexcept { return method_; }

std::string_view HttpRequest::target() const noexcept { return target_; }

std::string_view HttpRequest::version() const noexcept { return version_; }

void HttpRequest::addHeader(std::string name, std::string value) {
	headers_.add(std::move(name), std::move(value));
}

void HttpRequest::setHeader(std::string name, std::string value) {
	headers_.set(std::move(name), std::move(value));
}

std::string_view HttpRequest::header(std::string_view name) const noexcept {
	return headers_.get(name);
}

std::vector<std::string_view>
HttpRequest::headers(std::string_view name) const {
	return headers_.values(name);
}

void HttpRequest::setBody(std::string body) { body_ = std::move(body); }

std::string_view HttpRequest::body() const noexcept { return body_; }

} // namespace http
