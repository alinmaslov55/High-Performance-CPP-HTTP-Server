#include "http/network/TcpServer.hpp"

#include <iostream>
#include <stdexcept>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http/network/ClientConnection.hpp"
#include "http/http/HttpHeaders.hpp"

namespace http {

const char RESPONSE[] = "HTTP/1.1 200 OK\r\n"
						"Content-Type: text/plain\r\n"
						"Content-Length: 12\r\n"
						"Connection: close\r\n"
						"\r\n"
						"Hello World!";

TcpServer::TcpServer(int port, const Router& router)
	: server_socket_(Socket::create_tcp()), port_(port), router_(router) {
	server_socket_.setReuseAddress();
	server_socket_.bind(port_);
	server_socket_.listen();
}

void TcpServer::start() {
	std::cout << "[INFO] Server listening on port " << port_ << '\n';

	while (true) {
		ClientConnection connection(server_socket_.accept());

		std::cout << "[INFO] Client Connected\n";

		HttpRequest request;

		while(true){
			ParseResult result = connection.parseRequest(request);
			
			if(result == ParseResult::Incomplete){
				if(!connection.read()){
					std::cout << "[INFO] Client Disconnected\n";
					break;
				}
				continue;
			}

			HttpResponse response;

			if(result == ParseResult::Invalid){
				response.setStatus(HttpStatus::BadRequest);
				response.setBody("400 Bad Request");
				response.setHeader("Connection", "close");
				connection.send(response.serialize());
				break;
			}

			if(result == ParseResult::Complete){

				bool keepAlive{false};

				if(request.version() == "HTTP/1.1"){
					keepAlive = !HttpHeaders::equalsIgnoreCase(request.header("Connection"), "close");
				} else {
					keepAlive = HttpHeaders::equalsIgnoreCase(request.header("Connection"), "keep-alive");
				}

				response = router_.handle(request);
				
				if(keepAlive){
					response.setHeader("Connection", "keep-alive");
				} else {
					response.setHeader("Connection", "close");
				}

				connection.send(response.serialize());
				connection.consumeParsedRequest();

				if(!keepAlive){
					std::cout << "[INFO] Closing connection per protocol\n";
					break;
				}
			}
		}
	}
}

} // namespace http