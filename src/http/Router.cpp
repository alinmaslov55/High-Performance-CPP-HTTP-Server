#include <http/http/Router.hpp>

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
} // namespace utils

void Router::get(
	std::string path,
	Handler handler
) {
	routes_.push_back(
		Route{
			HttpMethod::GET,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::post(
	std::string path,
	Handler handler
) {
	routes_.push_back(
		Route{
			HttpMethod::POST,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::put(
	std::string path,
	Handler handler
) {
	routes_.push_back(
		Route{
			HttpMethod::PUT,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::patch(
	std::string path,
	Handler handler
) {
	routes_.push_back(
		Route{
			HttpMethod::PATCH,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::del(
	std::string path,
	Handler handler
) {
	routes_.push_back(
		Route{
			HttpMethod::DELETE,
			std::move(path),
			std::move(handler)
		}
	);
}

void Router::head(
	std::string path,
	Handler handler
) {
	routes_.push_back(
		Route{
			HttpMethod::HEAD,
			std::move(path),
			std::move(handler)
		}
	);
}

HttpResponse Router::handle(
	HttpRequest& request
) const {
	HttpResponse response;

	for(const auto& middleware: global_middlewares_){
		if(!middleware(request, response)){
			return response;
		}
	}

	bool pathFound = false;

	for (const Route& route : routes_) {

		bool matches = route.isPrefix ? request.path().rfind(route.path, 0) == 0
			: route.path == request.path();

		if (!matches) {
			continue;
		}
		pathFound = true;

		if (route.method != request.method()) {
			continue;
		}

		route.handler(request, response);

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
