#ifndef MODEL_USER_HPP
#define MODEL_USER_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace http {
namespace models {

struct User {
    std::string id;
    std::string name;
    std::string password_hash;
    std::string role;

    nlohmann::json toSafeJson() const {
        return {{"_id", id}, {"name", name}, {"role", role}};
    }
};

} // namespace models
} // namespace http

#endif // MODEL_USER_HPP