#include <gtest/gtest.h>
#include "http/http/HttpParser.hpp"
#include "http/http/HttpRequest.hpp"

namespace http_tests {

using namespace http;

TEST(HttpParserTest, ParsesCompleteGetRequest) {
    HttpParser parser;
    HttpRequest req;
    
    std::string data = "GET /api/users?status=active&sort=desc HTTP/1.1\r\n"
                       "Host: localhost:8080\r\n"
                       "Accept: application/json\r\n\r\n";
                       
    ParseResult result = parser.parse(data, req);
    
    EXPECT_EQ(result, ParseResult::Complete);
    EXPECT_EQ(req.method(), HttpMethod::GET);
    EXPECT_EQ(req.path(), "/api/users");
    EXPECT_EQ(req.query("status"), "active");
    EXPECT_EQ(req.query("sort"), "desc");
    EXPECT_EQ(req.header("Host"), "localhost:8080");
}

TEST(HttpParserTest, ParsesPostWithContentLength) {
    HttpParser parser;
    HttpRequest req;
    
    std::string data = "POST /submit HTTP/1.1\r\n"
                       "Content-Length: 11\r\n\r\n"
                       "Hello World";
                       
    ParseResult result = parser.parse(data, req);
    
    EXPECT_EQ(result, ParseResult::Complete);
    EXPECT_EQ(req.method(), HttpMethod::POST);
    EXPECT_EQ(req.body(), "Hello World");
    EXPECT_EQ(parser.consumedBytes(), data.size());
}

TEST(HttpParserTest, HandlesFragmentedPackets) {
    HttpParser parser;
    HttpRequest req;
    
    std::string buffer = "GET / HTTP/1.1\r\n";
    EXPECT_EQ(parser.parse(buffer, req), ParseResult::Incomplete);
    
    buffer += "Host: localhost\r\n";
    EXPECT_EQ(parser.parse(buffer, req), ParseResult::Incomplete);
    
    buffer += "\r\n";
    EXPECT_EQ(parser.parse(buffer, req), ParseResult::Complete);
    
    EXPECT_EQ(req.method(), HttpMethod::GET);
    EXPECT_EQ(req.header("Host"), "localhost");
}

TEST(HttpParserTest, ParsesChunkedTransferEncoding) {
    HttpParser parser;
    HttpRequest req;
    
    std::string data = "POST /stream HTTP/1.1\r\n"
                       "Transfer-Encoding: chunked\r\n\r\n"
                       "5\r\n"
                       "Hello\r\n"
                       "6\r\n"
                       " World\r\n"
                       "0\r\n\r\n";
                       
    ParseResult result = parser.parse(data, req);
    
    EXPECT_EQ(result, ParseResult::Complete);
    EXPECT_EQ(req.body(), "Hello World");
}

TEST(HttpParserTest, RejectsInvalidRequests) {
    HttpParser parser;
    HttpRequest req;
    
    std::string badData1 = "POST / HTTP/1.1\r\n"
                           "Content-Length: 5\r\n"
                           "Transfer-Encoding: chunked\r\n\r\n";
    EXPECT_EQ(parser.parse(badData1, req), ParseResult::Invalid);
    
    parser.reset();
    
    // Test 2: Invalid HTTP Version
    std::string badData2 = "GET / HTTP/1.0\r\n\r\n";
    EXPECT_EQ(parser.parse(badData2, req), ParseResult::Invalid);
}

} // namespace http_tests