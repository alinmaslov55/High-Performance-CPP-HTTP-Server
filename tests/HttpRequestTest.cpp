#include <gtest/gtest.h>
#include "http/http/HttpRequest.hpp"

using namespace http;

TEST(HttpRequestTest, SetAndGetMethod) {
    HttpRequest req;
    
    EXPECT_EQ(req.method(), HttpMethod::UNKNOWN);

    req.setMethod(HttpMethod::POST);
    EXPECT_EQ(req.method(), HttpMethod::POST);
}

TEST(HttpRequestTest, PathManagement) {
    HttpRequest req;
    req.setPath("/api/users");
    
    EXPECT_EQ(req.path(), "/api/users");
}

TEST(HttpRequestTest, HeaderManagement) {
    HttpRequest req;
    req.setHeader("Content-Type", "application/json");
    req.setHeader("Connection", "keep-alive");

    EXPECT_EQ(req.header("Content-Type"), "application/json");
    EXPECT_EQ(req.header("Connection"), "keep-alive");
    
    EXPECT_EQ(req.header("Authorization"), "");
}