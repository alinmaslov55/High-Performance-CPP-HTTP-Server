#include <http/http/HttpHeaders.hpp>

#include <algorithm>
#include <cctype>
#include <utility>

namespace http {

bool HttpHeaders::equalsIgnoreCase(std::string_view lhs,
								   std::string_view rhs) noexcept {
	if (lhs.size() != rhs.size()) {
		return false;
	}
	for (std::size_t i = 0; i < lhs.size(); ++i) {
		const unsigned char left = static_cast<unsigned char>(lhs[i]);
		const unsigned char right = static_cast<unsigned char>(rhs[i]);
		if (std::tolower(left) != std::tolower(right)) {
			return false;
		}
	}
	return true;
}

void HttpHeaders::add(std::string name, std::string value) {
	headers_.emplace_back(std::move(name), std::move(value));
}

void HttpHeaders::set(std::string name, std::string value) {
	std::erase_if(headers_, [&name](const Header &header) {
		return equalsIgnoreCase(header.first, name);
	});
	headers_.emplace_back(std::move(name), std::move(value));
}

std::string_view HttpHeaders::get(std::string_view name) const noexcept {
	for (const auto &[headerName, headerValue] : headers_) {
		if (equalsIgnoreCase(headerName, name)) {
			return headerValue;
		}
	}
	return {};
}

std::vector<std::string_view> HttpHeaders::values(std::string_view name) const {
	std::vector<std::string_view> result;
	for (const auto &[headerName, headerValue] : headers_) {
		if (equalsIgnoreCase(headerName, name)) {
			result.push_back(headerValue);
		}
	}
	return result;
}

bool HttpHeaders::contains(std::string_view name) const noexcept {
	for (const auto &[headerName, headerValue] : headers_) {
		if (equalsIgnoreCase(headerName, name)) {
			return true;
		}
	}
	return false;
}

std::size_t HttpHeaders::size() const noexcept { return headers_.size(); }

void HttpHeaders::clear() noexcept { headers_.clear(); }

const std::vector<HttpHeaders::Header>& HttpHeaders::all() const noexcept {
	return headers_;
}

} // namespace http