#include <gtest/gtest.h>
#include "http/http/HttpResponse.hpp"

namespace http_tests {

using namespace http;

TEST(HttpResponseTest, DefaultStatusIsOK) {
    HttpResponse res;
    EXPECT_EQ(res.status(), HttpStatus::OK);
}

TEST(HttpResponseTest, CanModifyStatus) {
    HttpResponse res;
    res.setStatus(HttpStatus::NotFound);
    EXPECT_EQ(res.status(), HttpStatus::NotFound);
}

TEST(HttpResponseTest, HeadersDelegation) {
    HttpResponse res;
    res.setHeader("Server", "HighPerf-Server");
    res.addHeader("Set-Cookie", "session=123");
    
    EXPECT_EQ(res.header("Server"), "HighPerf-Server");
    EXPECT_EQ(res.header("Set-Cookie"), "session=123");
}

TEST(HttpResponseTest, JsonHelperSetsHeaderAndBody) {
    HttpResponse res;
    res.json("{\"message\":\"success\"}");
    
    EXPECT_EQ(res.header("Content-Type"), "application/json");
    EXPECT_EQ(res.body(), "{\"message\":\"success\"}");
}

TEST(HttpResponseTest, SerializesProperly) {
    HttpResponse res;
    res.setStatus(HttpStatus::Created);
    res.setHeader("Content-Type", "text/plain");
    res.setBody("Hello World");
    
    std::string expected = "HTTP/1.1 201 Created\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 11\r\n\r\n"
                           "Hello World";
                           
    EXPECT_EQ(res.serialize(), expected);
}

TEST(HttpResponseTest, SerializeOverridesManualContentLength) {
    HttpResponse res;
    res.setHeader("Content-Length", "9999"); 
    res.setBody("Hi");
    
    std::string serialized = res.serialize();
    
    EXPECT_NE(serialized.find("Content-Length: 2\r\n"), std::string::npos);
    EXPECT_EQ(serialized.find("Content-Length: 9999\r\n"), std::string::npos);
}

} // namespace http_tests