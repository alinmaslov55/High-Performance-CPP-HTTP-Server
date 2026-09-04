#include <gtest/gtest.h>
#include "http/http/HttpHeaders.hpp"

namespace http_tests {

using namespace http;

TEST(HttpHeadersTest, CaseInsensitiveMatching) {
    HttpHeaders headers;
    headers.add("Content-Type", "application/json");

    EXPECT_TRUE(headers.contains("content-type"));
    EXPECT_TRUE(headers.contains("CONTENT-TYPE"));
    EXPECT_TRUE(headers.contains("CoNtEnT-TyPe"));
    
    EXPECT_EQ(headers.get("content-type"), "application/json");
    EXPECT_EQ(headers.get("CONTENT-TYPE"), "application/json");
}

TEST(HttpHeadersTest, AddAllowsDuplicates) {
    HttpHeaders headers;
    headers.add("Set-Cookie", "session_id=12345");
    headers.add("Set-Cookie", "theme=dark");

    EXPECT_EQ(headers.size(), 2);
    
    auto vals = headers.values("Set-Cookie");
    ASSERT_EQ(vals.size(), 2);
    EXPECT_EQ(vals[0], "session_id=12345");
    EXPECT_EQ(vals[1], "theme=dark");
}

TEST(HttpHeadersTest, SetOverwritesExisting) {
    HttpHeaders headers;
    headers.add("Connection", "keep-alive");
    
    headers.set("connection", "close"); 

    EXPECT_EQ(headers.size(), 1);
    EXPECT_EQ(headers.get("Connection"), "close");
}

TEST(HttpHeadersTest, GetReturnsEmptyForMissing) {
    HttpHeaders headers;
    
    EXPECT_FALSE(headers.contains("Authorization"));
    EXPECT_TRUE(headers.get("Authorization").empty());
}

TEST(HttpHeadersTest, ClearAndSizeManagement) {
    HttpHeaders headers;
    EXPECT_EQ(headers.size(), 0);
    
    headers.add("Host", "localhost");
    headers.add("Accept", "*/*");
    EXPECT_EQ(headers.size(), 2);
    
    headers.clear();
    EXPECT_EQ(headers.size(), 0);

}

} // namespace http_tests