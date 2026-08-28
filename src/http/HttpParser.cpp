#include <http/http/HttpParser.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <string>
#include <utility>

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

} // namespace utils

ParseResult HttpParser::parse(std::string_view data, HttpRequest &request) {
	while (true) {
		switch (state_) {
		case State::RequestLine: {

			if (consumed_ > data.size()) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const std::string_view remaining = data.substr(consumed_);

			const std::size_t lineEnd = remaining.find("\r\n");

			if (lineEnd == std::string_view::npos) {

				if (remaining.size() > MAX_REQUEST_LINE) {
					state_ = State::Error;
					return ParseResult::Invalid;
				}

				return ParseResult::Incomplete;
			}

			if (lineEnd > MAX_REQUEST_LINE) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const ParseResult result = parseRequestLine(remaining, request);

			if (result == ParseResult::Incomplete) {
				return ParseResult::Incomplete;
			}

			if (result == ParseResult::Invalid) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			consumed_ += lineEnd + 2;

			state_ = State::Headers;

			continue;
		}

		case State::Headers: {

			if (consumed_ > data.size()) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const std::string_view remaining = data.substr(consumed_);

			const std::size_t headersEnd = remaining.find("\r\n\r\n");

			if (headersEnd == std::string_view::npos) {

				if (remaining.size() > MAX_HEADER_SIZE) {
					state_ = State::Error;
					return ParseResult::Invalid;
				}

				return ParseResult::Incomplete;
			}

			if (headersEnd > MAX_HEADER_SIZE) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const std::string_view headers =
				remaining.substr(0, headersEnd + 2);

			const ParseResult result = parseHeaders(headers, request);

			if (result == ParseResult::Incomplete) {
				return ParseResult::Incomplete;
			}

			if (result == ParseResult::Invalid) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			consumed_ += headersEnd + 4;

			const std::string_view contentLength =
				request.header("Content-Length");

			const std::string_view transferEncoding =
				request.header("Transfer-Encoding");

			const bool hasContentLength = !contentLength.empty();

			const bool hasTransferEncoding = !transferEncoding.empty();

			if (hasContentLength && hasTransferEncoding) {

				state_ = State::Error;
				return ParseResult::Invalid;
			}

			if (hasTransferEncoding) {
				if (!utils::equalsIgnoreCase(utils::trimOws(transferEncoding),
											 "chunked")) {

					state_ = State::Error;
					return ParseResult::Invalid;
				}

				body_.clear();

				chunkSize_ = 0;
				chunkBytesRead_ = 0;

				state_ = State::ChunkSize;

				continue;
			}

			if (!hasContentLength) {

				bodySize_ = 0;
				request.setBody("");
				state_ = State::Complete;

				return ParseResult::Complete;
			}

			bodySize_ = 0;

			if (!utils::parseContentLength(contentLength, bodySize_)) {

				state_ = State::Error;
				return ParseResult::Invalid;
			}

			if (bodySize_ > MAX_BODY_SIZE) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			if (bodySize_ == 0) {
				request.setBody("");

				state_ = State::Complete;

				return ParseResult::Complete;
			}

			state_ = State::Body;

			continue;
		}

		case State::Body: {
			if (consumed_ > data.size()) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const std::size_t available = data.size() - consumed_;

			if (available < bodySize_) {
				return ParseResult::Incomplete;
			}

			request.setBody(std::string(data.substr(consumed_, bodySize_)));

			consumed_ += bodySize_;

			state_ = State::Complete;

			return ParseResult::Complete;
		}

		case State::ChunkSize: {

			if (consumed_ > data.size()) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const std::string_view remaining = data.substr(consumed_);

			const std::size_t lineEnd = remaining.find("\r\n");

			if (lineEnd == std::string_view::npos) {

				if (remaining.size() > MAX_CHUNK_LINE_SIZE) {

					state_ = State::Error;
					return ParseResult::Invalid;
				}

				return ParseResult::Incomplete;
			}

			if (lineEnd > MAX_CHUNK_LINE_SIZE) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			std::string_view line = remaining.substr(0, lineEnd);

			const std::size_t semicolon = line.find(';');

			if (semicolon != std::string_view::npos) {
				line = line.substr(0, semicolon);
			}

			line = utils::trimOws(line);

			if (!utils::parseChunkSize(line, chunkSize_)) {

				state_ = State::Error;
				return ParseResult::Invalid;
			}

			if (chunkSize_ > MAX_BODY_SIZE ||
				body_.size() > MAX_BODY_SIZE - chunkSize_) {

				state_ = State::Error;
				return ParseResult::Invalid;
			}

			consumed_ += lineEnd + 2;

			chunkBytesRead_ = 0;

			if (chunkSize_ == 0) {

				state_ = State::ChunkTrailer;

			} else {

				state_ = State::ChunkData;
			}

			continue;
		}

		case State::ChunkData: {

			if (consumed_ > data.size()) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const std::size_t available = data.size() - consumed_;

			const std::size_t remainingChunkBytes =
				chunkSize_ - chunkBytesRead_;

			const std::size_t bytesToCopy =
				std::min(available, remainingChunkBytes);

			if (bytesToCopy > 0) {

				body_.append(data.data() + consumed_, bytesToCopy);

				consumed_ += bytesToCopy;
				chunkBytesRead_ += bytesToCopy;
			}

			if (chunkBytesRead_ < chunkSize_) {
				return ParseResult::Incomplete;
			}

			state_ = State::ChunkDataCrlf;

			continue;
		}
		case State::ChunkDataCrlf: {

			if (consumed_ > data.size()) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const std::size_t available = data.size() - consumed_;

			if (available < 2) {
				return ParseResult::Incomplete;
			}

			if (data[consumed_] != '\r' || data[consumed_ + 1] != '\n') {

				state_ = State::Error;
				return ParseResult::Invalid;
			}

			consumed_ += 2;

			state_ = State::ChunkSize;

			continue;
		}

		case State::ChunkTrailer: {

			if (consumed_ > data.size()) {
				state_ = State::Error;
				return ParseResult::Invalid;
			}

			const std::string_view remaining = data.substr(consumed_);

			if (remaining.size() >= 2 && remaining[0] == '\r' &&
				remaining[1] == '\n') {

				consumed_ += 2;

				request.setBody(std::move(body_));

				state_ = State::Complete;

				return ParseResult::Complete;
			}

			const std::size_t trailerEnd = remaining.find("\r\n\r\n");

			if (trailerEnd == std::string_view::npos) {

				if (remaining.size() > MAX_HEADER_SIZE) {

					state_ = State::Error;
					return ParseResult::Invalid;
				}

				return ParseResult::Incomplete;
			}

			const std::string_view trailers =
				remaining.substr(0, trailerEnd + 2);

			std::size_t position = 0;
			std::size_t trailerCount = 0;

			while (position < trailers.size()) {
				const std::size_t lineEnd = trailers.find("\r\n", position);
				if (lineEnd == std::string_view::npos) {
					state_ = State::Error;
					return ParseResult::Invalid;
				}

				if (lineEnd == position) {
					break;
				}

				const std::string_view line =
					trailers.substr(position, lineEnd - position);

				const std::size_t colon = line.find(':');

				if (colon == std::string_view::npos) {
					state_ = State::Error;
					return ParseResult::Invalid;
				}

				const std::string_view name = line.substr(0, colon);

				const std::string_view value =
					utils::trimOws(line.substr(colon + 1));

				if (!utils::isValidHeaderName(name)) {
					state_ = State::Error;
					return ParseResult::Invalid;
				}

				if (!utils::isValidHeaderValue(value)) {
					state_ = State::Error;
					return ParseResult::Invalid;
				}

				++trailerCount;

				if (trailerCount > MAX_HEADERS) {
					state_ = State::Error;
					return ParseResult::Invalid;
				}

				position = lineEnd + 2;
			}

			consumed_ += trailerEnd + 4;

			request.setBody(std::move(body_));

			state_ = State::Complete;

			return ParseResult::Complete;
		}
		case State::Complete:
			return ParseResult::Complete;
		case State::Error:
			return ParseResult::Invalid;
		}
	}
}

ParseResult HttpParser::parseRequestLine(std::string_view data,
										 HttpRequest &request) const {

	const std::size_t lineEnd = data.find("\r\n");

	if (lineEnd == std::string_view::npos) {
		return ParseResult::Incomplete;
	}

	if (lineEnd > MAX_REQUEST_LINE) {
		return ParseResult::Invalid;
	}

	const std::string_view line = data.substr(0, lineEnd);

	const std::size_t firstSpace = line.find(' ');

	if (firstSpace == std::string_view::npos) {
		return ParseResult::Invalid;
	}

	const std::size_t secondSpace = line.find(' ', firstSpace + 1);

	if (secondSpace == std::string_view::npos) {
		return ParseResult::Invalid;
	}

	if (line.find(' ', secondSpace + 1) != std::string_view::npos) {

		return ParseResult::Invalid;
	}

	const std::string_view method = line.substr(0, firstSpace);

	const std::string_view target =
		line.substr(firstSpace + 1, secondSpace - firstSpace - 1);

	const std::string_view version = line.substr(secondSpace + 1);

	if (method.empty() || target.empty() || version.empty()) {

		return ParseResult::Invalid;
	}

	if (version != "HTTP/1.1") {
		return ParseResult::Invalid;
	}

	const HttpMethod parsedMethod = utils::parseMethod(method);

	if (parsedMethod == HttpMethod::UNKNOWN) {
		return ParseResult::Invalid;
	}

	request =
		HttpRequest(parsedMethod, std::string(target), std::string(version));

	return ParseResult::Complete;
}

ParseResult HttpParser::parseHeaders(std::string_view data,
									 HttpRequest &request) const {

	std::size_t position = 0;
	std::size_t headerCount = 0;

	while (position < data.size()) {
		const std::size_t lineEnd = data.find("\r\n", position);

		if (lineEnd == std::string_view::npos) {
			return ParseResult::Incomplete;
		}

		if (lineEnd == position) {
			return ParseResult::Complete;
		}

		const std::string_view line = data.substr(position, lineEnd - position);

		const std::size_t colon = line.find(':');

		if (colon == std::string_view::npos) {
			return ParseResult::Invalid;
		}

		const std::string_view name = line.substr(0, colon);

		const std::string_view value = utils::trimOws(line.substr(colon + 1));

		if (!utils::isValidHeaderName(name)) {
			return ParseResult::Invalid;
		}

		if (!utils::isValidHeaderValue(value)) {
			return ParseResult::Invalid;
		}

		++headerCount;

		if (headerCount > MAX_HEADERS) {
			return ParseResult::Invalid;
		}

		if (utils::equalsIgnoreCase(name, "Content-Length")) {

			const std::string_view existing = request.header("Content-Length");

			if (!existing.empty() && existing != value) {

				return ParseResult::Invalid;
			}
		}

		request.addHeader(std::string(name), std::string(value));

		position = lineEnd + 2;
	}

	return ParseResult::Incomplete;
}

void HttpParser::reset() noexcept {
	state_ = State::RequestLine;
	consumed_ = 0;
	bodySize_ = 0;

	chunkSize_ = 0;
	chunkBytesRead_ = 0;

	body_.clear();
}

std::size_t HttpParser::consumedBytes() const noexcept { return consumed_; }

} // namespace http