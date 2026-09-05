#include "http/http/HttpParser.hpp"
#include "http/http/HttpUtils.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace http {

// TODO: to split current method based on the switch
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
				remaining.substr(0, headersEnd + 4);

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
		HttpRequest(parsedMethod, target, version);

	const std::size_t questionMark = target.find('?');

	if(questionMark == std::string_view::npos){
		request.setPath(target);
		return ParseResult::Complete;
	}

	request.setPath(target.substr(0, questionMark));
        
	std::string_view queryString = target.substr(questionMark + 1);
	std::size_t start = 0;
	
	while (start < queryString.size()) {
		std::size_t amp = queryString.find('&', start);
		std::string_view pair = queryString.substr(start, amp - start);
		
		std::size_t eq = pair.find('=');
		if (eq != std::string_view::npos) {
			std::string_view key = pair.substr(0, eq);
			std::string_view value = pair.substr(eq + 1);
			request.addQuery(utils::urlDecode(key), utils::urlDecode(value));
		} else if (!pair.empty()) {
			request.addQuery(utils::urlDecode(pair), ""); // Key with no value
		}
		
		if (amp == std::string_view::npos) break;
		start = amp + 1;
	}

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