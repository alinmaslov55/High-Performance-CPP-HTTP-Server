#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <http/http/HttpRequest.hpp>
#include <http/http/HttpResponse.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace http {

class ClientConnection;
struct WebSocketFrame;

class Router {
public:
    using Handler = std::function<void(HttpRequest &, HttpResponse &)>;
    using Middleware = std::function<bool(HttpRequest &, HttpResponse &)>;
    using WsHandler =
        std::function<void(std::shared_ptr<ClientConnection> &, const WebSocketFrame &)>;

    Router() = default;

    void use(Middleware middleware);

    void get(std::string path, Handler handler);
    void post(std::string path, Handler handler);
    void put(std::string path, Handler handler);
    void patch(std::string path, Handler handler);
    void del(std::string path, Handler handler);
    void head(std::string path, Handler handler);

    void get(std::string path, std::vector<Middleware> middlewares, Handler handler);
    void post(std::string path, std::vector<Middleware> middlewares, Handler handler);
    void put(std::string path, std::vector<Middleware> middlewares, Handler handler);
    void patch(std::string path, std::vector<Middleware> middlewares, Handler handler);
    void del(std::string path, std::vector<Middleware> middlewares, Handler handler);
    void head(std::string path, std::vector<Middleware> middlewares, Handler handler);

    [[nodiscard]]
    HttpResponse handle(HttpRequest &request) const;

    void ws(std::string path, WsHandler handler);
    void handleWs(std::shared_ptr<ClientConnection> &connection, const WebSocketFrame &frame) const;

    void serveFiles(std::string mountPoint, std::string directory);

private:
    struct Route {
        HttpMethod method;
        std::string path;
        Handler handler;
        bool isPrefix{false};
        std::vector<Middleware> middlewares{};
    };

    std::vector<Route> routes_;
    std::vector<Middleware> global_middlewares_;
    std::unordered_map<std::string, WsHandler> ws_routes_;
};

} // namespace http

#endif // ROUTER_HPP
