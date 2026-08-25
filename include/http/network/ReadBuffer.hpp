#ifndef READ_BUFFER_HPP
#define READ_BUFFER_HPP

#include <cstddef>
#include <string>
#include <string_view>

namespace http {

class ReadBuffer {
  public:
	explicit ReadBuffer(std::size_t capacity = 8192);

	void append(const char *data, std::size_t size);

	[[nodiscard]]
	std::size_t size() const noexcept;

	[[nodiscard]]
	bool empty() const noexcept;

	[[nodiscard]]
	std::string_view data() const noexcept;

	void consume(std::size_t size);

	void clear() noexcept;

  private:
	std::string buffer_;
};

} // namespace http

#endif // READ_BUFFER_HPP