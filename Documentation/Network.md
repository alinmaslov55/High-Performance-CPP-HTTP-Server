# Network Module Documentation

- a high-performance, asynchronous, non-blocking TCP server architecture optimized for Linux
- utilizes epoll with Edge Triggering **EPOLLET** and the **SO_REUSEPORT** socket option
- zero mutex between worker threads

## Architecture

- One loop per thread
- Every Worker binds to the exact same port using SO_REUSEPORT

## Key Characteristics

- Zero-Copy Parsing - HTTP parses data without allocating memory on the heap
- Lock-Free Design - No mutex in favor of SO_REUSEPORT
- Edge-Triggered Polling - CPU wakes up only when state changes occur, reducing overhead on active sockets