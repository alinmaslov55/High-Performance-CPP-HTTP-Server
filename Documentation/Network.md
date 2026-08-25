# Network Module Documentation

## Overview
The `http` network module provides a C++ wrapper over standard sockets. It facilitates the creation of TCP servers, specifically tailored towards building an HTTP server. The module ensures safe resource management using RAII principles and supports move semantics for optimal performance.

## Architecture
Four classes:
1. **`Socket`**: A low-level RAII wrapper for socket file descriptors.
2. **`ReadBuffer`**: A utility for buffering incoming data from the network.
3. **`ClientConnection`**: Represents an active connection to a client, handling reading and sending data.
4. **`TcpServer`**: High-level server orchestrator that binds to a port and accepts incoming connections.

## Class Reference

### 1. `Socket`
The `Socket` class is responsible for managing the lifetime of a network socket file descriptor.

*   **Header:** `Socket.hpp`
*   **Properties:** Move-only (copy constructor and assignment are deleted). 
*   **Key Methods:**
    *   `static Socket create_tcp()`: Factory method that creates and returns a TCP (`AF_INET`, `SOCK_STREAM`) socket.
    *   `void setReuseAddress()`: Sets the `SO_REUSEADDR` socket option, useful for rapid server restarts.
    *   `void bind(uint16_t port)`: Binds the socket to `INADDR_ANY` on the specified port.
    *   `void listen(int backlog = SOMAXCONN)`: Marks the socket as a passive listening socket.
    *   `Socket accept()`: Blocks and accepts an incoming connection, returning a new `Socket` representing the client.
    *   `void close() noexcept`: Safely closes the underlying file descriptor.

### 2. `ReadBuffer`
The `ReadBuffer` handles the accumulation and consumption of incoming byte streams. It encapsulates a `std::string` to dynamically manage capacity.

*   **Header:** `ReadBuffer.hpp`
*   **Key Methods:**
    *   `explicit ReadBuffer(std::size_t capacity = 8192)`: Pre-allocates memory to minimize reallocations.
    *   `void append(const char *data, std::size_t size)`: Appends raw data received from a socket.
    *   `void consume(std::size_t size)`: Removes the first `size` bytes from the buffer. *(Note: Currently operates in O(N) time complexity due to `std::string::erase`)*.
    *   `std::string_view data() const noexcept`: Provides a lightweight, non-owning view of the buffer's contents.

### 3. `ClientConnection`
Wraps a client `Socket` and a `ReadBuffer` to manage an individual client session.

*   **Header:** `ClientConnection.hpp`
*   **Properties:** Move-only.
*   **Key Methods:**
    *   `explicit ClientConnection(Socket socket)`: Constructs a connection from an accepted socket.
    *   `bool read()`: Reads up to 8192 bytes from the socket into the internal `ReadBuffer`. Returns `false` if the connection is closed by the peer.
    *   `void send(const std::string &data)`: reliably sends all bytes in the string over the socket. Uses a loop to handle partial sends.
    *   `std::string_view data() const noexcept`: Retrieves the accumulated data from the read buffer.

### 4. `TcpServer`
The main entry point for the network module. It configures the listening socket and runs the accept loop.

*   **Header:** `TcpServer.hpp`
*   **Key Methods:**
    *   `explicit TcpServer(int port)`: Initializes the server, creates the socket, sets reuse, binds, and starts listening.
    *   `void start()`: Enters an infinite blocking loop. It accepts a client, reads the request, and responds with a hardcoded HTTP 200 "Hello World!" message.

## Future Improvements
*   **`ReadBuffer::consume` Performance:** The `consume` method relies on `std::string::erase`, which causes an $O(N)$ shift of characters. This can be optimized using a ring buffer or index offsets.
*   **Synchronous I/O:** `TcpServer::start()` currently operates in a single-threaded, synchronous loop. It processes one client at a time. To handle concurrent clients, integration with threads, `select/poll/epoll`, or asynchronous I/O (like `io_uring` or `Boost.Asio`) is necessary.