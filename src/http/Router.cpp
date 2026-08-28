#include <http/http/Router.hpp>

#include <utility>

namespace http {

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
	const HttpRequest& request
) const {
	bool pathFound = false;

	for (const Route& route : routes_) {

		if (route.path != request.target()) {
			continue;
		}

		pathFound = true;

		if (route.method != request.method()) {
			continue;
		}

		return route.handler(request);
	}


	HttpResponse response;

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

} // namespace http
