# High-Performance C++ HTTP Server

A blazing-fast, lock-free, asynchronous HTTP web framework built from scratch in modern C++20.

Designed for extreme throughput and low latency, this framework utilizes the **Reactor Pattern** with Linux `epoll`, Edge Triggering (`EPOLLET`), and `SO_REUSEPORT`. By implementing **Zero-Copy parsing** and a strictly lock-free core network engine, it completely eliminates OS-level context switches and mutex contention.

## Features

* **One-Loop-Per-Thread Architecture:** Kernel-level load balancing across CPU cores with `SO_REUSEPORT`.
* **Zero-Copy HTTP Parsing:** Fragment-safe state machine parser using `std::string_view` to eliminate heap allocations during network reads.
* **Express-style Routing:** Supports exact/prefix matching, HTTP verb mapping, and query parameter extraction.
* **Middleware Pipeline:** Supports both global middleware and route-specific middleware
* **Static File Server:** Built-in MIME-type inference and strict directory traversal protection.
* **Modern Tooling:** Automatic JSON body parsing (`nlohmann/json`), Chunked Transfer Encoding support, and URL decoding.
* **Lock-Free ThreadPool:** A custom C++20 atomic ring-buffer thread pool for offloading heavy application tasks.
* **Async Logging:** Logs with server activity containing Requests and Thread tasks
* **Stateless Authentication(JWT middleware):** Intercept incoming requests, parse the header for authorization, and reject unauthorized users
* **Environment Configuration:**

## Performance Benchmark

Tested on a Debian VM using `wrk` (12 threads, 400 concurrent connections, 30 seconds):

* **Throughput:** 26,054 Requests / Second
* **Latency:** 15.40 ms avg
* **Socket Errors:** 0

## Quick Start

```cpp
#include "http/network/TcpServer.hpp"
#include "http/http/Router.hpp"

using namespace http;

int main() {
    Router router;

    // Global Middleware
    router.use([](HttpRequest& req, HttpResponse& res) {
        res.setHeader("Server", "High-Perf-CPP");
        return true; 
    });

    // Basic Route
    router.get("/api/ping", [](HttpRequest& req, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
        res.json("{\"message\": \"pong\"}");
    });

    // Serve Static Files
    router.serveFiles("/static/", "./public");

    // Start Server on port 8080
    TcpServer server(8080, router);
    server.start();

    return 0;
}
```

## Prerequisites

- **OS**: Linux - requires *epoll* and *SO_REUSEPORT* kernel features
- **Compiler**: C++20 support
- **Build System**: CMake >= 3.14
- **MongoDb Installed**
- **jwt-cpp Installed**
- *Other Libraries are fetched by CMake*

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
find . \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +
```
