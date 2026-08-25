#include <cassert>

#include <http/http/HttpParser.hpp>

int main() {
	http::HttpParser parser;
	http::HttpRequest request;

	const auto result = parser.parseHeaders("Host: example.com\r\n"
											"Accept: application/js",
											request);

	assert(result == http::ParseResult::Incomplete);
}