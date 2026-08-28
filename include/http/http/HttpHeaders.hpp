#ifndef HTTP_HEADERS_HPP
#define HTTP_HEADERS_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace http {

class HttpHeaders {
  public:
	using Header = std::pair<std::string, std::string>;

	void add(std::string name, std::string value);
	void set(std::string name, std::string value);

	[[nodiscard]]
	std::string_view get(std::string_view name) const noexcept;

	[[nodiscard]]
	std::vector<std::string_view> values(std::string_view name) const;

	[[nodiscard]]
	bool contains(std::string_view name) const noexcept;

	[[nodiscard]]
	std::size_t size() const noexcept;

	void clear() noexcept;

    [[nodiscard]]
    const std::vector<Header>& all() const noexcept;
	static bool equalsIgnoreCase(std::string_view lhs,
								 std::string_view rhs) noexcept;
  private:

	std::vector<Header> headers_;
};

} // namespace http

#endif // HTPP_HEADERS_HPP