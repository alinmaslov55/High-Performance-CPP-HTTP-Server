#include "http/network/ReadBuffer.hpp"

#include <stdexcept>

namespace http {

ReadBuffer::ReadBuffer(std::size_t capacity) { buffer_.reserve(capacity); }

void ReadBuffer::append(const char *data, std::size_t size) {
	buffer_.append(data, size);
}

std::size_t ReadBuffer::size() const noexcept { return buffer_.size(); }

bool ReadBuffer::empty() const noexcept { return buffer_.empty(); }

std::string_view ReadBuffer::data() const noexcept {
	return static_cast<std::string_view>(buffer_);
}

void ReadBuffer::consume(std::size_t size) {
	if (size > buffer_.size()) {
		throw std::out_of_range(
			"Cannot consume more data than buffer contains");
	}

	// TODO solve O(n) complexity
	buffer_.erase(buffer_.begin(), buffer_.begin() + size);
}

void ReadBuffer::clear() noexcept { buffer_.clear(); }

} // namespace http