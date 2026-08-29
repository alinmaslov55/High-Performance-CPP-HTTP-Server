#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <http/http/HttpRequest.hpp>
#include <http/http/HttpResponse.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace http {

class Router {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    Router() = default;

    void get(std::string path, Handler handler);

    void post(std::string path, Handler handler);

    void put(std::string path, Handler handler);

    void patch(std::string path, Handler handler);

    void del(std::string path, Handler handler);

    void head(std::string path, Handler handler);

    [[nodiscard]]
    HttpResponse handle(const HttpRequest& request) const;

    // only for small files { .css .html .js}
    void serveFiles(std::string mountPoint, std::string directory);
private:
    struct Route{
        HttpMethod method;
        std::string path;
        Handler handler;
        bool isPrefix{false};
    };

    std::vector<Route> routes_;
};

} // namespace http

#endif // ROUTER_HPP