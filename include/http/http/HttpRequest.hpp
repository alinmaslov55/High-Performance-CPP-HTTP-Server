#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <http/http/HttpHeaders.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace http {

enum class HttpMethod {
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

class HttpRequest {
  public:
	HttpRequest() = default;

	HttpRequest(HttpMethod method, std::string target, std::string version);

	[[nodiscard]]
	HttpMethod method() const noexcept;

	void setMethod(HttpMethod method);

	[[nodiscard]]
	std::string_view target() const noexcept;

	[[nodiscard]]
	std::string_view version() const noexcept;

	void addHeader(std::string name, std::string value);

	void setHeader(std::string name, std::string value);

	[[nodiscard]]
	std::string_view header(std::string_view name) const noexcept;

	[[nodiscard]]
	std::vector<std::string_view> headers(std::string_view name) const;

	void setBody(std::string body);

	[[nodiscard]]
	std::string_view body() const noexcept;

	[[nodiscard]]
	std::string_view path() const noexcept;

	[[nodiscard]]
	std::string_view query(std::string_view key) const noexcept;

	void setPath(std::string path);
	void addQuery(std::string key, std::string value);

	void setJson(nlohmann::json json_body);

	[[nodiscard]]
	const nlohmann::json& json() const;

	[[nodiscard]]
	bool hasJson() const noexcept;
  private:
	HttpMethod method_ = HttpMethod::UNKNOWN;

	std::string target_;
	std::string version_;

	HttpHeaders headers_;

	std::string body_;
	std::string path_;
	std::unordered_map<std::string, std::string> queries_;

	nlohmann::json json_body_;
	bool has_json_{false};
};

} // namespace http

#endif // HTTP_REQUEST_HPP