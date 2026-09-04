#include <gtest/gtest.h>
#include "http/http/HttpRequest.hpp"

namespace http_tests {

using namespace http;
using json = nlohmann::json;

TEST(HttpRequestTest, ConstructorAndCoreProperties) {
    HttpRequest req(HttpMethod::GET, "/api/users?sort=asc", "HTTP/1.1");
    
    EXPECT_EQ(req.method(), HttpMethod::GET);
    EXPECT_EQ(req.target(), "/api/users?sort=asc");
    EXPECT_EQ(req.version(), "HTTP/1.1");

    req.setMethod(HttpMethod::POST);
    EXPECT_EQ(req.method(), HttpMethod::POST);
}

TEST(HttpRequestTest, PathAndQueryParameters) {
    HttpRequest req;
    
    req.setPath("/api/users");
    EXPECT_EQ(req.path(), "/api/users");

    req.addQuery("sort", "desc");
    req.addQuery("limit", "10");

    EXPECT_EQ(req.query("sort"), "desc");
    EXPECT_EQ(req.query("limit"), "10");
    
    EXPECT_TRUE(req.query("page").empty());
}

TEST(HttpRequestTest, HeaderDelegation) {
    HttpRequest req;
    
    req.setHeader("Host", "localhost:8080");
    EXPECT_EQ(req.header("Host"), "localhost:8080");

    req.addHeader("Accept", "text/html");
    req.addHeader("Accept", "application/json");

    auto accepts = req.headers("Accept");
    ASSERT_EQ(accepts.size(), 2);
    EXPECT_EQ(accepts[0], "text/html");
    EXPECT_EQ(accepts[1], "application/json");
}

TEST(HttpRequestTest, BodyManagement) {
    HttpRequest req;
    
    EXPECT_TRUE(req.body().empty());
    
    req.setBody("Plain text payload");
    EXPECT_EQ(req.body(), "Plain text payload");
}

TEST(HttpRequestTest, JsonIntegration) {
    HttpRequest req;
    
    EXPECT_FALSE(req.hasJson());

    json payload = {
        {"name", "Alice"},
        {"age", 28}
    };

    req.setJson(payload);
    
    EXPECT_TRUE(req.hasJson());
    
    const json& stored_json = req.json();
    EXPECT_EQ(stored_json.value("name", ""), "Alice");
    EXPECT_EQ(stored_json.value("age", 0), 28);
}

} // namespace http_tests
