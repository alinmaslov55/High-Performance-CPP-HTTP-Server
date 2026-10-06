# Controllers and Models Documentation

The Controllers and Models module serves as the application's core business logic layer. It bridges the gap between raw HTTP routing, middleware interception, and database persistence, providing a structured MVC-like architecture for API endpoints.

This module is responsible for extracting network payloads, enforcing business rules, securely hashing credentials, and serializing safe data structures back to the client.

## Architecture

* **Separation of Concerns:** Controllers handle strictly HTTP-related semantics (parsing JSON, setting status codes, returning errors) and delegate all database operations to injected Repositories.
* **Middleware Integration:** Route protection is applied directly during route registration. The `AuthMiddleware::requireAuth` function wraps sensitive endpoints, ensuring unauthorized requests never reach the controller logic.
* **Secure Serialization:** Data models strictly control their own outward-facing representations. The `toSafeJson()` method ensures sensitive fields (like password hashes) are stripped before the object reaches the HTTP response layer.

## Core Components

* **`UserController`:** The orchestrator for all user-related endpoints (`/api/users` and `/api/login`). It securely parses incoming `nlohmann::json` payloads, interacts with the `UserRepository` for CRUD operations, and leverages `CryptoUtils` and `JwtUtils` for authentication.
* **`User` (Model):** A lightweight C++ struct representing a MongoDB document. It maps standard fields (`id`, `name`, `password_hash`, `role`) and provides the serialization logic necessary to safely convert the internal C++ state into a client-facing JSON object.

## Endpoint Lifecycle (Authentication Example)

1. **Routing & Extraction:** A POST request hits `/api/login`. The `Router` dispatches it to `UserController::loginUser`. The helper `extractJson` safely parses the body, gracefully falling back to a `400 Bad Request` if the JSON is malformed or missing required fields.
2. **Database Lookup:** The controller queries the `UserRepository` to find the user by their plaintext username.
3. **Cryptographic Verification:** If the user exists, `CryptoUtils::verifyPassword` executes a secure, constant-time comparison between the provided plaintext password and the stored bcrypt/Argon2 hash.
4. **Token Generation:** Upon successful verification, `JwtUtils::generateToken` creates a stateless JWT containing the user's unique `_id` and `role`. The controller serializes this token into the `HttpResponse` with a `200 OK` status.

## Future Architectural Improvements

### 1. Dedicated Service Layer

Currently, `UserController` handles both HTTP semantics (status codes, JSON formatting) and business logic (password hashing, token generation). Introducing a dedicated `UserService` layer would completely decouple business rules from the HTTP context, making the core logic highly unit-testable without requiring mocked `HttpRequest` or `HttpResponse` objects.

### 2. Cursor-Based Pagination

The `getAllUsers` endpoint currently implements pagination using MongoDB's `skip` and `limit` operations. As the database scales, large `skip` values become a significant O(N) performance bottleneck because the database must scan and discard all skipped documents. Migrating to Cursor-Based Pagination (querying where `_id > last_seen_id`) would guarantee O(1) pagination performance regardless of collection size.

### 3. DTO Validation Pipeline

Manual JSON field validation (e.g., `!payload.contains("name")`) works for simple endpoints but becomes difficult to maintain for complex nested objects. Implementing a Data Transfer Object (DTO) pattern with a JSON Schema validation pipeline would allow the server to automatically reject invalid types, missing fields, or out-of-bounds values before the request ever reaches the controller logic.
