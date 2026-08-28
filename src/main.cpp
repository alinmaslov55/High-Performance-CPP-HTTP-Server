#include <http/http/Router.hpp>

int main() {

	http::Router router;

	router.get(
		"/hello",
		[](const http::HttpRequest&) {

			http::HttpResponse response;

			response.setStatus(
				http::HttpStatus::OK
			);

			response.setHeader(
				"Content-Type",
				"text/plain"
			);

			response.setBody(
				"Hello World"
			);

			return response;
		}
	);

}