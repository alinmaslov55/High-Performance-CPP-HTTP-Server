# High-Performance C++ HTTP Server

A fast, lock-free, asynchronous HTTP web framework built from scratch in modern C++20.

Designed for extreme throughput and low latency, this framework utilizes the **Reactor Pattern** with Linux `epoll`, Edge Triggering (`EPOLLET`), and `SO_REUSEPORT`. By implementing Zero-Copy parsing for HTTP/1.1 and integrating a highly optimized nghttp2 state machine for multiplexed HTTP/2, it fully terminates its own TLS encryption while safely offloading heavy application tasks to a custom lock-free ThreadPool.

## Features

* **HTTP/2 & Stream Multiplexing**: Native ALPN protocol negotiation and binary framing using `libnghttp2`. Seamlessly handles concurrent streams over a single TCP connection.

* **Native TLS/HTTPS**: Integrated OpenSSL for strict encryption and secure WebSocket upgrades (`wss://`).

* **One-Loop-Per-Thread Architecture**: Kernel-level load balancing across CPU cores with `SO_REUSEPORT`.

* **Zero-Copy HTTP Parsing**: Fragment-safe state machine parser using `std::string_view` to eliminate heap allocations during network reads.

* **Lock-Free ThreadPool**: A custom C++20 atomic ring-buffer thread pool for offloading heavy application tasks (like MongoDB queries) without blocking the Epoll event loop.

* **Express-style Routing**: Supports exact/prefix matching, HTTP verb mapping, and query/path parameter extraction.

* **WebSockets & Telemetry**: Full-duplex WebSocket support featuring a detached background service that broadcasts real-time server telemetry (connections, RPS, queue depth) directly to clients.

* **Middleware Pipeline**: Supports global and route-specific middleware. Pre-configured with **Redis-backed Rate Limiting** and **JWT Stateless Authentication**.

* **Static File Server**: Built-in MIME-type inference, default `index.html` resolution, and strict directory traversal protection.

## Performance Benchmark

Tested on a Debian VM using `wrk` (12 threads, 400 concurrent connections, 30 seconds):

* **Throughput:** 23,043 Requests / Second
* **Data Transferred**: 1.48 GB
* **Latency:** 15.40 ms avg
* **Socket Errors:** 0
* **ThreadPool Queue Bottlenecks**: 0

### Live Dashboard

* Navigate to `https://localhost:8080/index.html` to view the live WebSocket telemetry dashboard, which visualizes active connections, HTTP throughput, and the background task queue in real time

## Quick Start

```cpp
#include "http/network/TcpServer.hpp"
#include "http/http/Router.hpp"

using namespace http;

int main() {
    Router router;

    // Global Middleware
    router.use([](HttpRequest& req, HttpResponse& res) {
        res.setHeader("Server", "High-Perf-CPP-H2");
        return true; 
    });

    // Basic REST Endpoint
    router.get("/api/ping", [](HttpRequest& req, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
        res.json("{\"message\": \"pong\"}");
    });

    // Secure WebSocket Upgrade
    router.ws("/api/stream", [](std::shared_ptr<ClientConnection>& conn, const WebSocketFrame& frame) {
        conn->sendWebSocketMessage("Hello from the C++ Event Loop!");
    });

    // Serve Static Files (Auto-resolves to index.html)
    router.serveFiles("/static/", "./public");

    // Start Server on port 8080 (Requires certs/server.crt and certs/server.key)
    TcpServer server(8080, router);
    server.start();

    return 0;
}
```

## Prerequisites

- **OS**: Linux - requires *epoll* and *SO_REUSEPORT* kernel features
- **Compiler**: C++20 support
- **Build System**: CMake >= 3.20
- **Dependencies**: installed via `vcpkg`
    1. nghttp2
    2. OpenSSL
    3. MongoDB CXX Driver
    4. Redis++
    5. jwt-cpp
    6. libsodium
    7. nlohmann/json

## Directory Structure

* **src/network/** - Epoll, TCP sockets, and `nghttp2` connection states.
* **src/http/** - Custom HTTP/1.1 parser, Router, and Request/Response models.
* **src/concurrency/** - Lock-free ring-buffer ThreadPool.
* **src/middlewares/** - Redis rate limiting and JWT auth injection.
* **public/** - Static assets and the telemetry HTML dashboard.

## Build and Run Commands

- Dev Build

```bash
cmake -S . -B build

# after mongodb(installed with vcpkg)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build
./build/http_server
```

- Docker Container

```bash
# to build
docker compose up --build

# to run detached mode
docker compose up -d

# show logs
docker compose logs -f

# application logs only
docker compose logs -f api_server

# to stop
docker compose down
```

- Release Build

```bash
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build_release
cmake --build build_release
```

- Unit Tests

```bash
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

- Code Formatting

```bash
find src/ include/ tests/ -type f \
    \( -name "*.cpp" -o -name "*.hpp" \) \
    -exec clang-format -i {} +
```
