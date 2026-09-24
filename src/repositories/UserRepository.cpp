#include "http/repositories/UserRepository.hpp"
#include "http/utils/Logger.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <mongocxx/options/find.hpp>

namespace http {
namespace repositories {

UserRepository::UserRepository(db::MongoPool& db_pool) : db_pool_(db_pool) {}

std::optional<models::User> UserRepository::findByUsername(const std::string& username) {
    try {
        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];
        
        auto query = bsoncxx::builder::stream::document{}
            << "name" << username
            << bsoncxx::builder::stream::finalize;

        auto result = collection.find_one(query.view());

        if (result) {
            auto view = result->view();
            models::User user;
            user.id = view["_id"].get_oid().value.to_string();
            user.name = std::string(view["name"].get_string().value);
            
            if (view["password_hash"]) {
                user.password_hash = std::string(view["password_hash"].get_string().value);
            }
            if (view["role"]) {
                user.role = std::string(view["role"].get_string().value);
            } else {
                user.role = "user";
            }
            return user;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Database error in findByUsername: {}", e.what());
    }
    return std::nullopt;
}

std::optional<models::User> UserRepository::findById(const std::string& id) {
    try {
        bsoncxx::oid document_id(id);

        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];
        
        auto query = bsoncxx::builder::stream::document{}
            << "_id" << document_id
            << bsoncxx::builder::stream::finalize;

        auto result = collection.find_one(query.view());

        if (result) {
            auto view = result->view();
            models::User user;
            user.id = view["_id"].get_oid().value.to_string();
            user.name = std::string(view["name"].get_string().value);
            
            if (view["password_hash"]) {
                user.password_hash = std::string(view["password_hash"].get_string().value);
            }
            if (view["role"]) {
                user.role = std::string(view["role"].get_string().value);
            } else {
                user.role = "user";
            }
            return user;
        }
    } catch (const bsoncxx::exception& e) {
        LOG_ERROR("Invalid OID format in findById: {}", id);
    } catch (const std::exception& e) {
        LOG_ERROR("Database error in findById: {}", e.what());
    }
    return std::nullopt;
}

bool UserRepository::createUser(const std::string& name, const std::string& password_hash, const std::string& role){
    try {
        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];
        
        auto doc = bsoncxx::builder::stream::document{}
            << "name" << name
            << "password_hash" << password_hash
            << "role" << role
            << bsoncxx::builder::stream::finalize;
            
        collection.insert_one(doc.view());
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Database error in createUser: {}", e.what());
        return false;
    }
}

bool UserRepository::updateUser(const std::string& id, const nlohmann::json& updates) {
    try {
        bsoncxx::oid document_id(id);

        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];

        auto filter = bsoncxx::builder::stream::document{} 
            << "_id" << document_id
            << bsoncxx::builder::stream::finalize;

        bsoncxx::document::value update_doc = bsoncxx::from_json(updates.dump());

        auto update = bsoncxx::builder::stream::document{}
            << "$set" << update_doc.view()
            << bsoncxx::builder::stream::finalize;
        
        auto result = collection.update_one(filter.view(), update.view());

        return result && result->matched_count() > 0;
    } catch (const bsoncxx::exception& e) {
        LOG_ERROR("Invalid OID format in updateUser: {}", id);
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Database error in updateUser: {}", e.what());
        return false;
    }
}

bool UserRepository::deleteUser(const std::string& id) {
    try {
        bsoncxx::oid document_id(id);

        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];

        auto filter = bsoncxx::builder::stream::document{}
            << "_id" << document_id
            << bsoncxx::builder::stream::finalize;
        
        auto result = collection.delete_one(filter.view());

        return result && result->deleted_count() > 0;
    } catch (const bsoncxx::exception& e) {
        LOG_ERROR("Invalid OID format in deleteUser: {}", id);
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Database error in deleteUser: {}", e.what());
        return false;
    }
}

std::vector<models::User> UserRepository::getAllUsers(int limit, int skip, const std::string& sort_field, int sort_order) {
    std::vector<models::User> users;
    try {
        auto conn = db_pool_.acquire();
        auto collection = (*conn)["test_db"]["users"];

        mongocxx::options::find opts;
        if (limit > 0) opts.limit(limit);
        if (skip > 0) opts.skip(skip);
        if (!sort_field.empty()) {
            opts.sort(bsoncxx::builder::stream::document{}
                << sort_field << sort_order << bsoncxx::builder::stream::finalize
            );
        }

        auto cursor = collection.find({}, opts);
        for (auto&& doc : cursor) {
            auto view = doc;
            models::User user;
            user.id = view["_id"].get_oid().value.to_string();
            user.name = std::string(view["name"].get_string().value);
            user.role = view["role"] ? std::string(view["role"].get_string().value) : "user";
            users.push_back(user);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Database error in getAllUsers: {}", e.what());
    }
    return users;
}

} // namespace repositories
} // namespace http