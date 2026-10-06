# Concurrency Module Documentation

A high-performance, low-latency concurrency engine designed to offload blocking operations (such as MongoDB queries, complex routing, or heavy computation) from the main `epoll` network loop.

By utilizing modern C++20 synchronization primitives and a pre-allocated circular buffer, this `ThreadPool` avoids the heavy OS-level context switching and dynamic memory allocation overhead associated with traditional `std::mutex` and `std::queue` implementations.

## Architecture

* **Bounded Ring Buffer:** Uses a fixed-capacity (1024) circular array instead of a dynamically growing queue, ensuring strict memory limits and cache-friendly contiguous memory access.
* **Spinlock Synchronization:** Replaces standard OS-level mutexes with a lightweight `std::atomic_flag` spinlock, drastically reducing contention latency for rapid enqueue/dequeue operations.
* **C++20 Semaphores:** Leverages `std::counting_semaphore` for producer-consumer signaling, providing faster and more deterministic thread wake-ups compared to legacy `std::condition_variable` implementations.
* **Atomic Telemetry:** Maintains a lock-free `pending_tasks_` counter for real-time health monitoring and WebSocket dashboard telemetry.

## Core Components

* **`ThreadPool`:** The orchestration class that initializes the worker threads and manages the lifecycle of the background compute engine.
* **`ring_buffer_` (`std::vector<std::function<void()>>`):** A pre-allocated array acting as a circular queue. Tasks are inserted at the `tail_` index and extracted from the `head_` index.
* **`lock_` (`std::atomic_flag`):** A highly optimized, hardware-level atomic boolean used to grant exclusive access to the buffer indices for mere nanoseconds during task insertion or extraction.
* **`tasks_available_` & `space_available_` (`std::counting_semaphore`):** Dual semaphores that elegantly handle backpressure. If the pool hits the `CAPACITY` limit of 1024 pending tasks, the `space_available_` semaphore naturally prevents the network loop from overflowing the buffer.

## Task Lifecycle

1. **Enqueue (Producer):**
   A network `Worker` thread receives a fully parsed HTTP request. It acquires the `space_available_` semaphore, briefly acquires the `atomic_flag` spinlock, writes a C++ lambda (the route handler) into the `ring_buffer_` at the `tail_` index, and releases the spinlock. It then increments the `pending_tasks_` atomic counter and signals the `tasks_available_` semaphore.
2. **Wake & Dequeue (Consumer):**
   An idle background thread blocked on `tasks_available_` wakes up. It acquires the spinlock, extracts the lambda from the `head_` index, clears the buffer slot, and releases the spinlock. It then signals `space_available_` to let the network loop know a slot has freed up.
3. **Execution:**
   The background thread safely executes the task (e.g., querying the database and generating the `HttpResponse`) entirely outside the lock, decrements `pending_tasks_`, and then loops back to wait for the next signal.

## Future Architectural Improvements

### 1. True Lock-Free MPMC Queue

While the `std::atomic_flag` spinlock is incredibly fast, it still requires threads to briefly halt if they collide. Upgrading the internal data structure to a true Multi-Producer Multi-Consumer (MPMC) lock-free queue (using atomic Compare-And-Swap on individual node slots) would eliminate the spinlock entirely, allowing true concurrent enqueues and dequeues.

### 2. Work-Stealing Algorithm

Currently, all worker threads contend for a single centralized `ring_buffer_`. As core counts scale (e.g., 32+ cores), this central queue becomes a bottleneck. Implementing a work-stealing scheduler—where each worker thread has its own local queue and only "steals" from other workers when empty—would drastically improve CPU cache locality and scale linearly with core count.

### 3. Dynamic Thread Scaling

The pool is currently initialized with a fixed number of threads (`DEFAULT_THREADS = 4`). Implementing dynamic elasticity—where the pool monitors `pending_tasks_` and automatically spawns additional temporary worker threads during high-traffic spikes, then reaps them during idle periods—would improve resource efficiency on shared cloud instances.
