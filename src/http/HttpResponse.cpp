#include <http/http/HttpResponse.hpp>

#include <utility>

namespace http {

HttpResponse::HttpResponse()
	: status_(HttpStatus::OK) {}

void HttpResponse::setStatus(HttpStatus status) noexcept {
	status_ = status;
}

HttpStatus HttpResponse::status() const noexcept {
	return status_;
}

void HttpResponse::addHeader(
	std::string name,
	std::string value
) {
	headers_.add(
		std::move(name),
		std::move(value)
	);
}

void HttpResponse::setHeader(
	std::string name,
	std::string value
) {
	headers_.set(
		std::move(name),
		std::move(value)
	);
}

std::string_view HttpResponse::header(
	std::string_view name
) const noexcept {
	return headers_.get(name);
}

void HttpResponse::setBody(std::string body) {
	body_ = std::move(body);
}

std::string_view HttpResponse::body() const noexcept {
	return body_;
}

std::string HttpResponse::reasonPhrase(
	const HttpStatus& status
) const noexcept {

	switch (status) {

		case HttpStatus::OK:
			return "OK";

		case HttpStatus::Created:
			return "Created";

		case HttpStatus::NoContent:
			return "No Content";

		case HttpStatus::BadRequest:
			return "Bad Request";

		case HttpStatus::Unauthorized:
			return "Unauthorized";

		case HttpStatus::Forbidden:
			return "Forbidden";

		case HttpStatus::NotFound:
			return "Not Found";

		case HttpStatus::MethodNotAllowed:
			return "Method Not Allowed";

		case HttpStatus::InternalServerError:
			return "Internal Server Error";

		case HttpStatus::NotImplemented:
			return "Not Implemented";

		case HttpStatus::ServiceUnavailable:
			return "Service Unavailable";
	}

	return "Unknown";
}

std::string HttpResponse::serialize() const {

	std::string result;

	result.reserve(
		128 + body_.size()
	);

	result += "HTTP/1.1 ";

	const int statusCode =
		static_cast<int>(status_);

	result += std::to_string(statusCode);

	result += ' ';

	result += reasonPhrase(status_);

	result += "\r\n";

	for (const auto& [name, value] : headers_.all()) {

		/*
		 * Content-Length is generated automatically below.
		 *
		 * Don't serialize a user-supplied Content-Length here.
		 */

		if (HttpHeaders::equalsIgnoreCase(
				name,
				"Content-Length")) {

			continue;
		}

		result += name;
		result += ": ";
		result += value;
		result += "\r\n";
	}

	result += "Content-Length: ";
	result += std::to_string(body_.size());
	result += "\r\n";

	result += "\r\n";

	result += body_;


	return result;
}

} // namespace http
