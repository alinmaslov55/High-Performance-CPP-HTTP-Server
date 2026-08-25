#ifndef READ_BUFFER_HPP
#define READ_BUFFER_HPP

#include <cstddef>
#include <string>
#include <string_view>

namespace http {

/**
 * @brief Utility to accumulate bytes from a connection
 */
class ReadBuffer {
  public:
	explicit ReadBuffer(std::size_t capacity = 8192);

	/**
	 * @brief Appends raw data byte sto the end of buffer
	 * @param data Pointer to raw byte array
	 * @param size Numbe rof bytes to be appended
	 */
	void append(const char *data, std::size_t size);

	/**
	 * @brief Retrieve current number of bytes in buffer
	 * @return The size of unconsumed data
	 */
	[[nodiscard]]
	std::size_t size() const noexcept;

	/**
	 * @brief CHecks if buffer is empty
	 * @return true if buffer is empty, false otherwise
	 */
	[[nodiscard]]
	bool empty() const noexcept;

	/**
	 * @brief Provides read-only access to the buffer
	 * @return std::string_view of the buffer
	 */
	[[nodiscard]]
	std::string_view data() const noexcept;

	/**
	 * @brief Remove specified number of bytes from front of buffer
	 * @param size numbe rof bytes to remove
	 * @throws std::runtime_error
	 */
	void consume(std::size_t size);

	/**
	 * @brief Clears data from buffer withoput memory release
	 */
	void clear() noexcept;

  private:
	std::string buffer_;
};

} // namespace http

#endif // READ_BUFFER_HPP