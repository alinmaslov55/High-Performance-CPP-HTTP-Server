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

std::string_view HttpRequest::path() const noexcept { 
    return path_; 
}

std::string_view HttpRequest::query(std::string_view key) const noexcept {
    auto it = queries_.find(std::string(key));
    if (it != queries_.end()) {
        return it->second;
    }
    return {};
}

void HttpRequest::setPath(std::string path) { 
    path_ = std::move(path); 
}

void HttpRequest::addQuery(std::string key, std::string value) {
    queries_.emplace(std::move(key), std::move(value));
}

void HttpRequest::setJson(nlohmann::json json_body) {
    json_body_ = std::move(json_body);
    has_json_ = true;
}

const nlohmann::json& HttpRequest::json() const {
    return json_body_;
}

bool HttpRequest::hasJson() const noexcept {
    return has_json_;
}

void HttpRequest::setMethod(HttpMethod method) {
    method_ = method;
}

} // namespace http
