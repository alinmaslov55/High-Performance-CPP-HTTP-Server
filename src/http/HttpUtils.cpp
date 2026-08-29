#include "http/http/HttpUtils.hpp"

namespace http {
namespace utils {

HttpMethod parseMethod(std::string_view method) {
	if (method == "GET")
		return HttpMethod::GET;

	if (method == "POST")
		return HttpMethod::POST;

	if (method == "PUT")
		return HttpMethod::PUT;

	if (method == "PATCH")
		return HttpMethod::PATCH;

	if (method == "DELETE")
		return HttpMethod::DELETE;

	if (method == "HEAD")
		return HttpMethod::HEAD;

	if (method == "OPTIONS")
		return HttpMethod::OPTIONS;

	if (method == "CONNECT")
		return HttpMethod::CONNECT;

	if (method == "TRACE")
		return HttpMethod::TRACE;

	return HttpMethod::UNKNOWN;
}

std::string_view trimOws(std::string_view value) {
	while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
		value.remove_prefix(1);
	}

	while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
		value.remove_suffix(1);
	}

	return value;
}

bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs) {
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

bool isValidHeaderName(std::string_view name) {
	if (name.empty()) {
		return false;
	}

	for (const unsigned char character : name) {
		const bool valid =
			std::isalnum(character) || character == '!' || character == '#' ||
			character == '$' || character == '%' || character == '&' ||
			character == '\'' || character == '*' || character == '+' ||
			character == '-' || character == '.' || character == '^' ||
			character == '_' || character == '`' || character == '|' ||
			character == '~';

		if (!valid) {
			return false;
		}
	}

	return true;
}

bool isValidHeaderValue(std::string_view value) {
	for (const unsigned char character : value) {
		if (character == '\r' || character == '\n') {
			return false;
		}

		if (character < 0x20 && character != '\t') {
			return false;
		}

		if (character == 0x7F) {
			return false;
		}
	}

	return true;
}

bool parseContentLength(std::string_view value, std::size_t &result) {
	if (value.empty()) {
		return false;
	}
	for (const unsigned char character : value) {
		if (character < '0' || character > '9') {
			return false;
		}
	}

	const auto [pointer, error] =
		std::from_chars(value.data(), value.data() + value.size(), result);

	return error == std::errc{} && pointer == value.data() + value.size();
}

bool parseChunkSize(std::string_view value, std::size_t &result) {
	if (value.empty()) {
		return false;
	}

	result = 0;

	for (const unsigned char character : value) {
		std::size_t digit = 0;

		if (character >= '0' && character <= '9') {

			digit = static_cast<std::size_t>(character - '0');

		} else if (character >= 'a' && character <= 'f') {

			digit = static_cast<std::size_t>(character - 'a' + 10);

		} else if (character >= 'A' && character <= 'F') {

			digit = static_cast<std::size_t>(character - 'A' + 10);

		} else {
			return false;
		}

		if (result > (std::numeric_limits<std::size_t>::max() - digit) / 16) {
			return false;
		}

		result = result * 16 + digit;
	}

	return true;
}

std::string urlDecode(std::string_view value){
	std::string result;
    result.reserve(value.size());
    
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            result += ' ';
        } else if (value[i] == '%' && i + 2 < value.size()) {
            auto fromHex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };

            const int d1 = fromHex(value[i + 1]);
            const int d2 = fromHex(value[i + 2]);

            if (d1 != -1 && d2 != -1) {
                result += static_cast<char>((d1 << 4) | d2);
                i += 2;
            } else {
                result += '%';
            }
        } else {
            result += value[i];
        }
    }
    
    return result;
}

} // namespace utils
} // namespace http