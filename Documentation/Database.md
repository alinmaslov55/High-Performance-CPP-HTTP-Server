# Database Module Documentation

A thread-safe, high-performance database connection manager designed to seamlessly integrate with the background `ThreadPool`. It wraps the official MongoDB C++ Driver (`mongocxx`) to provide robust, concurrent database access without blocking the core network loop.

By utilizing connection pooling, this module ensures that the overhead of TCP handshakes, TLS negotiation, and database authentication occurs only once during server startup, rather than on every incoming HTTP request.

## Architecture

* **Pre-allocated Socket Pool:** Maintains a persistent pool of active TCP connections to the MongoDB server.
* **RAII Resource Management:** Connection borrowing and returning is strictly governed by C++ RAII (Resource Acquisition Is Initialization) semantics, making resource leaks physically impossible.
* **Thread-Safe Dispatch:** Designed to be safely accessed by multiple `ThreadPool` background workers simultaneously without requiring application-level mutexes (relying on the driver's internal thread-safe pool).
* **Singleton-Pattern Ready:** Enforces non-copyability (via `delete` semantics) to guarantee only one connection pool exists per database URI, preventing socket exhaustion.

## Core Components

* **`MongoPool`:** The central orchestrator. Initialized with a standard MongoDB URI (defaulting to `mongodb://localhost:27017`), it manages the lifecycle of the underlying `mongocxx::pool` and `mongocxx::uri`.
* **`acquire()`:** The primary interface for background workers. It requests a connection from the pool and blocks the calling worker thread only if the pool is completely exhausted and waiting for a connection to be returned.
* **`mongocxx::pool::entry`:** The RAII wrapper returned by `acquire()`. It acts as a smart pointer to a live database connection. When the entry goes out of scope at the end of a route handler, its destructor automatically and instantly releases the connection back to the pool.

## Query Lifecycle

1. **Borrow (Acquire):** An HTTP route handler executing inside a background `ThreadPool` worker calls `db->acquire()`. The worker is immediately handed an active, authenticated `mongocxx::pool::entry`.
2. **Execute:** The worker uses the acquired connection to execute CRUD operations (e.g., verifying a user, fetching a document) natively in BSON format.
3. **Return (Release):** Once the route handler finishes and the `entry` object goes out of scope, C++ destroys the RAII wrapper. The underlying socket is kept alive and safely returned to the `MongoPool` for the next worker thread to use.

## Future Architectural Improvements

### 1. Integrated Redis Caching Tier

Currently, every database request hits MongoDB directly. By injecting a Redis connection pool alongside `MongoPool`, the application could implement a write-through or read-aside caching layer. The `ThreadPool` workers could check Redis for hot data (like user profiles or authorization roles) in O(1) time before falling back to the heavier MongoDB query.

### 2. Circuit Breaker & Retry Logic

If the MongoDB instance restarts or drops connections, active `ThreadPool` workers might encounter unhandled exceptions that drop the HTTP request. Implementing a Circuit Breaker pattern with automatic retry logic would allow the pool to gracefully intercept connection failures, attempt to re-establish the socket, and retry the query without failing the client's HTTP request.

### 3. Dynamic Pool Configuration

The current pool relies on the default sizes dictated by the `mongocxx` driver. Exposing pool configuration parameters (minimum/maximum connections, connection timeout limits, and idle max life) via environment variables or a `config.json` file would allow DevOps engineers to dynamically tune the database backpressure based on specific hardware limits.
