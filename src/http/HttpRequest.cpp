#include <http/http/HttpRequest.hpp>

#include <cctype>

namespace http {

HttpRequest::HttpRequest(HttpMethod method, std::string target,
						 std::string version)
	: method_(method), target_(std::move(target)),
	  version_(std::move(version)) {}

HttpMethod HttpRequest::method() const noexcept { return method_; }

std::string_view HttpRequest::target() const noexcept { return target_; }

std::string_view HttpRequest::version() const noexcept { return version_; }

std::string_view HttpRequest::header(std::string_view name) const noexcept {
	const std::string normalized = normalizeHeaderName(name);
	const auto it = headers_.find(std::string(normalized));

	if (it == headers_.end()) {
		return "";
	}

	return it->second;
}

bool HttpRequest::hasHeader(std::string_view name) const noexcept {
	const std::string normalized = normalizeHeaderName(name);
	return headers_.contains(std::string(normalized));
}

void HttpRequest::setHeader(std::string name, std::string value) {
	name = normalizeHeaderName(name);
	headers_[std::move(name)] = std::move(value);
}

void HttpRequest::setBody(std::string body) { body_ = std::move(body); }

std::string HttpRequest::normalizeHeaderName(std::string_view name) {
	std::string normalized;

	normalized.reserve(name.size());

	for (const unsigned char character : name) {
		normalized.push_back(static_cast<char>(std::tolower(character)));
	}

	return normalized;
}

} // namespace http