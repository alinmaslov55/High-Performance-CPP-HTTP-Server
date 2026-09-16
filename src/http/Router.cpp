#include <http/http/Router.hpp>

#include <string>
#include <utility>
#include <fstream>
#include <filesystem>

namespace http {

namespace fs = std::filesystem;

namespace utils{

static std::string getMimeType(const std::string& extension) {
	if (extension == ".html" || extension == ".htm") return "text/html";
	if (extension == ".css") return "text/css";
	if (extension == ".js") return "application/javascript";
	if (extension == ".png") return "image/png";
	if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
	if (extension == ".svg") return "image/svg+xml";
	if (extension == ".json") return "application/json";
	if (extension == ".txt") return "text/plain";
	return "application/octet-stream"; // default binary type
}

std::vector<std::string_view> splitPath(std::string_view s, char delim){
    std::vector<std::string_view> result;
    size_t start = 0;
    while(start < s.size()){
        size_t end = s.find(delim, start);
        if(end == std::string_view::npos){
            if(start < s.size() && !s.substr(start).empty()){
                result.push_back(s.substr(start));
            }
            break;
        }
        if(end > start){
            result.push_back(s.substr(start, end - start));
        }
        start = end + 1;
    }
    return result;
}

bool matchDynamicPath(const std::string& route_path, const std::string_view req_path, HttpRequest& req) {
	if (route_path == req_path) return true;

	if(route_path.find(':') == std::string::npos) return false;

	auto route_segs = splitPath(route_path, '/');
	auto req_segs = splitPath(req_path, '/');
	if (route_segs.size() != req_segs.size()) return false;

	std::vector<std::pair<std::string, std::string>> extracted_params;
	for(size_t i = 0; i < route_segs.size(); ++i) {
		if(!route_segs[i].empty() && route_segs[i][0] == ':') {
			extracted_params.emplace_back(
				std::string(route_segs[i].substr(1)),
				std::string(req_segs[i])
			);
		} else if (route_segs[i] != req_segs[i]) {
			return false;
		}
	}

	for(const auto& [k, v] : extracted_params) {
		req.setParam(k, v);
	}
	return true;
}


} // namespace utils

void Router::get( std::string path, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::GET,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::get(std::string path, std::vector<Middleware> middlewares, Handler handler) {
	routes_.push_back(
		Route{
			HttpMethod::GET,
			std::move(path),
			std::move(handler),
			false,
			std::move(middlewares)
		}
	);
}

void Router::post( std::string path, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::POST,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::post( std::string path, std::vector<Middleware> middlewares, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::POST,
			std::move(path),
			std::move(handler),
			false,
			std::move(middlewares)
		}
	);
}

void Router::put( std::string path, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::PUT,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::put( std::string path, std::vector<Middleware> middlewares, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::PUT,
			std::move(path),
			std::move(handler),
			false,
			std::move(middlewares)
		}
	);
}

void Router::patch( std::string path, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::PATCH,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::patch( std::string path, std::vector<Middleware> middlewares, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::PATCH,
			std::move(path),
			std::move(handler),
			false,
			std::move(middlewares)
		}
	);
}

void Router::del( std::string path, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::DELETE,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::del( std::string path, std::vector<Middleware> middlewares, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::DELETE,
			std::move(path),
			std::move(handler),
			false,
			std::move(middlewares)
		}
	);
}

void Router::head( std::string path, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::HEAD,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::head( std::string path, std::vector<Middleware> middlewares, Handler handler ) {
	routes_.push_back(
		Route{
			HttpMethod::HEAD,
			std::move(path),
			std::move(handler),
			false,
			std::move(middlewares)
		}
	);
}

HttpResponse Router::handle( HttpRequest& request ) const {
	HttpResponse response;

	for(const auto& middleware: global_middlewares_){
		if(!middleware(request, response)){
			return response;
		}
	}

	bool pathFound = false;

	for (const Route& route : routes_) {

		bool matches{false};

		if(route.isPrefix) {
			matches = request.path().rfind(route.path, 0) == 0;
		} else {
			matches = utils::matchDynamicPath(route.path, request.path(), request);
		}

		if (!matches) {
			continue;
		}
		pathFound = true;

		if (route.method != request.method()) {
			continue;
		}

		bool middleware_passed = true;
		for(const auto& middleware: route.middlewares){
			if(!middleware(request, response)){
				middleware_passed = false;
				break;
			}
		}

		if(middleware_passed){
			route.handler(request, response);
		}
		return response;
	}

	if (pathFound) {
		response.setStatus(
			HttpStatus::MethodNotAllowed
		);
		response.setBody(
			"Method Not Allowed"
		);
		return response;
	}

	response.setStatus(
		HttpStatus::NotFound
	);
	response.setBody(
		"Not Found"
	);
	return response;
}

void Router::serveFiles(std::string mountPoint, std::string directory){
    if (mountPoint.back() != '/') mountPoint += '/';

    Handler fileHandler = [mountPoint, directory](HttpRequest& req, HttpResponse& res) {

        std::string_view reqPath = req.path();
        std::string relativePath = std::string(reqPath.substr(mountPoint.size()));

        if (relativePath.find("..") != std::string::npos) {
            res.setStatus(HttpStatus::Forbidden);
            res.setBody("403 Forbidden");
            return;
        }

        fs::path fullPath = fs::path(directory) / relativePath;

        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            res.setStatus(HttpStatus::NotFound);
            res.setBody("404 File Not Found");
            return;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string buffer(size, '\0');
        if (file.read(buffer.data(), size)) {
            res.setStatus(HttpStatus::OK);
            res.setHeader("Content-Type", utils::getMimeType(fullPath.extension().string()));
            res.setBody(std::move(buffer));
        } else {
            res.setStatus(HttpStatus::InternalServerError);
            res.setBody("500 Internal Server Error");
        }
    };

    routes_.push_back(Route{HttpMethod::GET, std::move(mountPoint), std::move(fileHandler), true});
}
void Router::use(Middleware middleware){
	global_middlewares_.push_back(std::move(middleware));
}

} // namespace http
