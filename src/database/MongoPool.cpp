#include "http/database/MongoPool.hpp"

namespace db {

MongoPool::MongoPool(const std::string& uri_string):
    uri_(mongocxx::uri(uri_string)),
    pool_(uri_)
{
}

mongocxx::pool::entry MongoPool::acquire() {
    return pool_.acquire();
}

} // namespace db