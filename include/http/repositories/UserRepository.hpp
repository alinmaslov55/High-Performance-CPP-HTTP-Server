#ifndef USER_REPOSITORY_HPP
#define USER_REPOSITORY_HPP

#include "http/database/MongoPool.hpp"
#include "http/models/User.hpp"

#include <optional>
#include <string>
#include <vector>

namespace http {
namespace repositories {

class UserRepository {
public:
    explicit UserRepository(db::MongoPool& db_pool);

    std::vector<models::User> getAllUsers(int limit, int skip, const std::string& sort_field, int sort_order);
    std::optional<models::User> findById(const std::string& id);
    std::optional<models::User> findByUsername(const std::string& username);
    bool createUser(const std::string& name, const std::string& password_hash, const std::string& role);
    bool updateUser(const std::string& id, const nlohmann::json& updates);
    bool deleteUser(const std::string& id);

private:
    db::MongoPool& db_pool_;
};

} // namespace repositories
} // namespace http

#endif // USER_REPOSITORY_HPP