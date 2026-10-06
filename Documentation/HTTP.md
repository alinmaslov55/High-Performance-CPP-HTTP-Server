# HTTP Module Documentation

- responsible for safely parsing raw TCP byte streams into structured HTTP objects, routing requests to user-defined handlers, and serializing responses

## Request Lifecycle

1. **Parsing** (`HttpParser & HttpHeaders`): The parser operates as a fragment-safe state machine, reading raw bytes from the network layer. It processes the request line, headers, and body iteratively, safely handling network fragmentation and Chunked Transfer Encoding.
2. **Structure** (`HttpRequest`): Network data is mapped into a rich `HttpRequest` model. This object standardizes the HTTP method into a strong enum (`HttpMethod`), stores URL path parameters, parses query strings, and directly decodes JSON payloads using `nlohmann::json`.
3. **Routing** (`Router`): The router evaluates the incoming request against registered endpoints. It sequentially executes any Global Middleware, followed by Route-Specific Middleware. If all middleware returns `true`, the request is passed to the primary endpoint handler or upgraded to a WebSocket connection.
4. **Serialization** (`HttpResponse`): The user-defined handler populates the `HttpResponse` object. The response dynamically calculates its `Content-Length`, maps the `HttpStatus` enum to a standard reason phrase, and serializes the entire HTTP payload into a string for the socket to transmit.

## Components

- **HttpParser**: Maintains internal states (e.g., `RequestLine`, `Headers`, `ChunkData`) to parse streams without blocking. It enforces strict memory limits (`MAX_HEADER_SIZE`, `MAX_BODY_SIZE`) to protect against buffer overflow and DoS attacks.

- **HttpRequestHttpResponse**: High-level abstractions that eliminate the need to interact with raw text. They provide direct getter/setter methods for headers and seamlessly integrate with JSON.

- **Router**: The central dispatcher for the application. It maintains REST endpoints in a `std::vector<Route>`, handles static file serving (`serveFiles`), and manages protocol upgrades via a dedicated `ws_routes_` map

## Future Improvements

1. Radix Tree (Trie) Routing
Currently, `Router` evaluates REST endpoints using a linear scan across `std::vector<Route>`. As the API grows, this becomes an `O(N)` bottleneck on every single request. Migrating the routing table to a Radix Tree will guarantee `O(K)` lookup times (where K is the URL length) and natively optimize dynamic path parameter extraction (e.g., `/api/users/:id`).
2. Scatter-Gather I/O for Response Serialization
`HttpResponse::serialize()` returns a contiguous `std::string`. If an endpoint serves a 5MB JSON payload, this design forces the engine to allocate a new 5MB buffer and copy the body into it just to prepend the HTTP headers. By transitioning to a `writev` (scatter-gather) pattern, the network layer can transmit the static status line, the header block, and the body from their original memory locations simultaneously without any intermediate heap copying.
3. O(1) Header Lookups
HttpHeaders relies on a `std::vector<std::pair<std::string, std::string>>`. Querying a specific header (like `Authorization` or `Connection`) requires an `O(N)` linear scan using case-insensitive string comparisons. Replacing this internal vector with an unordered map utilizing a custom case-insensitive hash—or a Perfect Hash Function (PHF) for standard RFC headers—will significantly reduce CPU cycles per request.
4. SIMD-Accelerated Parsing
The `HttpParser` reads byte-by-byte to find `\r\n` boundaries. Integrating SIMD (Single Instruction, Multiple Data) intrinsics (like AVX2 or SSE4.2) would allow the parser to scan 16 or 32 bytes simultaneously, drastically accelerating the parsing of large header blocks or chunked payloads.
