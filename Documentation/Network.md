# Network Module Documentation

A high-performance, asynchronous, non-blocking TCP server architecture explicitly optimized for Linux environments. It relies on a one-loop-per-thread concurrency model, utilizing Edge-Triggered `epoll` (`EPOLLET`) and the `SO_REUSEPORT` socket option to completely eliminate kernel-level and user-level thread contention during network I/O.

By binding every worker thread to the exact same port, the Linux kernel natively load-balances incoming TCP handshakes across all available CPU cores. This negates the need for a dedicated "acceptor" thread and allows the engine to run without a single cross-thread routing mutex.

## Architecture

* **One loop per thread:** Each worker thread maintains its own isolated event loop.
* **Kernel-level load balancing:** Every Worker binds to the exact same port using `SO_REUSEPORT`.
* **Zero-Copy Parsing:** HTTP parses data without allocating memory on the heap.
* **Lock-Free Design:** Zero cross-thread mutexes for incoming connections in favor of `SO_REUSEPORT`.
* **Edge-Triggered Polling:** The CPU wakes up only when state changes occur (`EPOLLET`), reducing overhead on active sockets.

## Core Components

* **`TcpServer` & `Worker`:** The orchestrator of the network layer. `TcpServer` bootstraps the background `ThreadPool` and spawns isolated `Worker` threads. Each `Worker` maintains its own `Epoll` instance, its own listening `Socket`, and an independent `unordered_map` of active `ClientConnection` objects.
* **`Socket`:** A strict RAII wrapper around raw Linux file descriptors. It encapsulates POSIX socket configurations, ensuring strict lifecycle management and enforcing zero-downtime restarts via `SO_REUSEADDR` and horizontal scaling via `SO_REUSEPORT`.
* **`Epoll`:** A streamlined C++ interface for the Linux `epoll` API. By utilizing `EPOLLET` (Edge Triggering) alongside `EPOLLONESHOT`, it guarantees that the CPU only wakes up when a socket state physically changes, preventing the "thundering herd" problem and minimizing CPU context-switch overhead on active connections.
* **`ClientConnection`:** The central state machine for active sockets. It manages OpenSSL TLS lifecycles (`doHandshake`), HTTP/2 multiplexing via `nghttp2`, and WebSocket framing. It safely bridges the asynchronous network layer and the multithreaded application layer by inheriting `std::enable_shared_from_this`, utilizing an internal `h2_mutex_` to prevent background ThreadPool workers from colliding with the Epoll loop when injecting multiplexed HTTP/2 responses.
* **`ReadBuffer`:** A dynamic memory buffer that accumulates bytes directly from `SSL_read`. It is designed to safely hold partial TCP fragments until a full HTTP request or WebSocket frame is complete, facilitating zero-copy parsing by exposing a `std::string_view` for downstream parsers.

## Request Data Flow

1. **Acceptance:** An incoming TCP connection triggers an `EPOLLIN` event on a specific `Worker` thread. The worker calls `accept()`, wraps the file descriptor in a non-blocking `Socket`, assigns it an OpenSSL context, and registers it to the worker's local `epoll` instance.
2. **TLS Handshake:** The socket state enters `doHandshake()`. `Epoll` will continuously bounce the connection between `EPOLLIN` and `EPOLLOUT` until OpenSSL successfully negotiates the encryption keys and extracts the ALPN protocol (HTTP/1.1 vs HTTP/2).
3. **Read & Delegate:** Once secure, raw encrypted bytes are pulled from the kernel, decrypted by OpenSSL, and appended to the `ReadBuffer`. The bytes are then routed to either the HTTP/1.1 `HttpParser`, the HTTP/2 `nghttp2_session_mem_recv` bridge, or the WebSocket frame decoder.
4. **Offload:** Fully assembled requests are handed off to the detached `ThreadPool` for application routing, allowing the `Worker` thread to immediately return to the `Epoll` wait state to service other active sockets.

## Future Architectural Improvements

### 1. Hashed Hierarchical Timing Wheel (O(1) Timeout Management)

Currently, idle connection reaping is handled by `sweepIdleConnections()`, which linearly iterates over the `active_connections_` hash map. Under extreme load (e.g., 100,000 concurrent websockets), this O(N) sweep blocks the event loop. Replacing this with a Timing Wheel data structure would reduce connection timeout management to an O(1) operation.

### 2. Object Pooling for Client Connections

Creating a `std::shared_ptr<ClientConnection>` for every new TCP socket incurs a heap allocation. By implementing a lock-free Object Pool (or a slab allocator) for `ClientConnection` instances on a per-worker basis, the engine could eliminate dynamic memory allocation during connection establishment, significantly improving CPU cache locality and reducing garbage collection overhead.

### 3. `io_uring` Integration

While Edge-Triggered `epoll` is highly performant, it still requires system calls (`SSL_read`, `SSL_write`) that trigger user-to-kernel context switches. Refactoring the event loop to support Linux's modern `io_uring` API would allow the server to batch network reads and writes asynchronously via shared memory rings, pushing throughput even closer to the physical limits of the NIC.
