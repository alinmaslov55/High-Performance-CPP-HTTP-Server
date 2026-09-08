# HTTP Module Documentation

- responsible for safely parsing raw TCP byte streams into structured HTTP objects, routing requests to user-defined handlers, and serializing responses

## Architecture

- Linear flow

    1. Parsing - The HttpParser reads raw bytes from the network ReadBuffer. Instead of copying strings, it assigns std::string_view pointers directly to the buffer memory
    2. Structure - The data is organized into an HttpRequest object, which has parsed headers, query parameters, and decoded JSON bodies
    3. Routing - The Router intercepts the request, runs it through any Global or Route-Specific Middleware, and matches it to the correct handler
    4. Serialization - The handler generates an HttpResponse, which dynamically calculates its own Content-Length and serializes itself back into a raw string for the network layer to transmit.

