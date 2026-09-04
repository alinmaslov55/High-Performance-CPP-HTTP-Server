#include <gtest/gtest.h>
#include "http/http/HttpUtils.hpp"

namespace http_tests {

using namespace http;
using namespace http::utils;

TEST(HttpUtilsTest, ParseMethod) {
    EXPECT_EQ(parseMethod("GET"), HttpMethod::GET);
    EXPECT_EQ(parseMethod("POST"), HttpMethod::POST);
    
    EXPECT_EQ(parseMethod("get"), HttpMethod::UNKNOWN);
    EXPECT_EQ(parseMethod("INVALID"), HttpMethod::UNKNOWN);
}

TEST(HttpUtilsTest, TrimOptionalWhitespace) {
    EXPECT_EQ(trimOws("  hello  "), "hello");
    EXPECT_EQ(trimOws("\t\tvalue\t"), "value");
    EXPECT_EQ(trimOws("no_spaces"), "no_spaces");
    EXPECT_EQ(trimOws("   "), "");
    EXPECT_EQ(trimOws(""), "");
}

TEST(HttpUtilsTest, EqualsIgnoreCase) {
    EXPECT_TRUE(equalsIgnoreCase("Content-Type", "content-type"));
    EXPECT_TRUE(equalsIgnoreCase("CONTENT-LENGTH", "content-length"));
    EXPECT_FALSE(equalsIgnoreCase("Host", "Hosts")); // Length mismatch
    EXPECT_FALSE(equalsIgnoreCase("Host", "Gost"));
}

TEST(HttpUtilsTest, ValidateHeaderNames) {
    EXPECT_TRUE(isValidHeaderName("Content-Type"));
    EXPECT_TRUE(isValidHeaderName("X-Custom-Header_123"));
    
    EXPECT_FALSE(isValidHeaderName("Content Type"));
    EXPECT_FALSE(isValidHeaderName("Content:Type"));
    EXPECT_FALSE(isValidHeaderName(""));
}

TEST(HttpUtilsTest, ValidateHeaderValues) {
    EXPECT_TRUE(isValidHeaderValue("application/json"));
    EXPECT_TRUE(isValidHeaderValue("value with spaces"));
    EXPECT_TRUE(isValidHeaderValue("value\twith\ttabs"));
    
    EXPECT_FALSE(isValidHeaderValue("value\r\nBad-Header: true"));
    EXPECT_FALSE(isValidHeaderValue("value\n"));
}

TEST(HttpUtilsTest, ParseContentLength) {
    std::size_t len = 0;
    
    EXPECT_TRUE(parseContentLength("1024", len));
    EXPECT_EQ(len, 1024);
    
    EXPECT_TRUE(parseContentLength("0", len));
    EXPECT_EQ(len, 0);
    
    EXPECT_FALSE(parseContentLength("1024x", len));
    EXPECT_FALSE(parseContentLength("-5", len));
    EXPECT_FALSE(parseContentLength("", len));
}

TEST(HttpUtilsTest, ParseChunkSize) {
    std::size_t size = 0;
    
    EXPECT_TRUE(parseChunkSize("A", size)); // 10
    EXPECT_EQ(size, 10);
    
    EXPECT_TRUE(parseChunkSize("1a", size)); // 26
    EXPECT_EQ(size, 26);
    
    EXPECT_TRUE(parseChunkSize("1A", size)); // 26
    EXPECT_EQ(size, 26);
    
    EXPECT_FALSE(parseChunkSize("1g", size)); // Invalid hex
}

TEST(HttpUtilsTest, UrlDecode) {
    EXPECT_EQ(urlDecode("hello+world"), "hello world");
    EXPECT_EQ(urlDecode("hello%20world"), "hello world");
    EXPECT_EQ(urlDecode("%2Fapi%2Fusers%3Fid%3D42"), "/api/users?id=42");
    
    EXPECT_EQ(urlDecode("100%"), "100%");
    EXPECT_EQ(urlDecode("invalid%2Z"), "invalid%2Z");
}

} // namespace http_tests