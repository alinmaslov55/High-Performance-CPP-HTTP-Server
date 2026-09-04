#include <gtest/gtest.h>
#include "http/http/Router.hpp"

namespace http_tests {

using namespace http;

HttpRequest createRequest(HttpMethod method, const std::string& path) {
    HttpRequest req;
    req.setMethod(method);
    req.setPath(path);
    return req;
}

TEST(RouterTest, ExactRouteMatching) {
    Router router;
    
    router.get("/api/users", [](HttpRequest&, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
        res.setBody("users_data");
    });

    HttpRequest req = createRequest(HttpMethod::GET, "/api/users");
    HttpResponse res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::OK);
    EXPECT_EQ(res.body(), "users_data");
}

TEST(RouterTest, Handles404NotFound) {
    Router router;
    
    router.get("/api/users", [](HttpRequest&, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
    });

    HttpRequest req = createRequest(HttpMethod::GET, "/api/unknown");
    HttpResponse res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::NotFound);
}

TEST(RouterTest, Handles405MethodNotAllowed) {
    Router router;
    
    router.post("/api/data", [](HttpRequest&, HttpResponse& res) {
        res.setStatus(HttpStatus::Created);
    });

    HttpRequest req = createRequest(HttpMethod::GET, "/api/data");
    HttpResponse res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::MethodNotAllowed);
}

TEST(RouterTest, GlobalMiddlewareExecution) {
    Router router;
    
    router.use([](HttpRequest&, HttpResponse& res) {
        res.setHeader("X-Global", "active");
        return true; 
    });
    
    router.use([](HttpRequest& req, HttpResponse& res) {
        if (req.path() == "/admin") {
            res.setStatus(HttpStatus::Forbidden);
            return false;
        }
        return true;
    });

    router.get("/home", [](HttpRequest&, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
    });
    router.get("/admin", [](HttpRequest&, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
    });

    auto reqHome = createRequest(HttpMethod::GET, "/home");
    HttpResponse resHome = router.handle(reqHome);
    EXPECT_EQ(resHome.status(), HttpStatus::OK);
    EXPECT_EQ(resHome.header("X-Global"), "active");

    HttpRequest reqAdmin = createRequest(HttpMethod::GET, "/admin");
    HttpResponse resAdmin = router.handle(reqAdmin);
    EXPECT_EQ(resAdmin.status(), HttpStatus::Forbidden);
}

TEST(RouterTest, RouteSpecificMiddleware) {
    Router router;
    
    Router::Middleware requireAuth = [](HttpRequest& req, HttpResponse& res) {
        if (req.header("Authorization") != "secret") {
            res.setStatus(HttpStatus::Unauthorized);
            return false;
        }
        return true;
    };

    router.get("/api/secure", {requireAuth}, [](HttpRequest&, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
        res.setBody("secure_payload");
    });

    HttpRequest reqFail = createRequest(HttpMethod::GET, "/api/secure");
    EXPECT_EQ(router.handle(reqFail).status(), HttpStatus::Unauthorized);

    HttpRequest reqPass = createRequest(HttpMethod::GET, "/api/secure");
    reqPass.setHeader("Authorization", "secret");
    
    HttpResponse resPass = router.handle(reqPass);
    EXPECT_EQ(resPass.status(), HttpStatus::OK);
    EXPECT_EQ(resPass.body(), "secure_payload");
}

TEST(RouterTest, ServeFilesDirectoryTraversalProtection) {
    Router router;
    router.serveFiles("/static", "./public");

    HttpRequest req = createRequest(HttpMethod::GET, "/static/../etc/passwd");
    HttpResponse res = router.handle(req);
    
    EXPECT_EQ(res.status(), HttpStatus::Forbidden);
}

} // namespace http_tests