#ifndef MONGO_POOL_HPP
#define MONGO_POOL_HPP

#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>

#include <memory>
#include <string>

namespace db {

constexpr auto DEFAULT_MONGO_URI = "mongodb://localhost:27017";

/**
 * @brief Thread-safe MongoDB connection pool manager
 */
class MongoPool {
public:
    explicit MongoPool(const std::string& uri_string = DEFAULT_MONGO_URI);

    MongoPool(const MongoPool&) = delete;
    MongoPool& operator=(const MongoPool&) = delete;

    /**
     * @brief Acquire a connection from the pool
     * @return A RAII wrapper of mongocxx::pool::entry object representing the acquired connection
     */
    mongocxx::pool::entry acquire();
private:

    mongocxx::uri uri_;
    mongocxx::pool pool_;
};

} // namespace db

#endif // MONGO_POOL_HPP