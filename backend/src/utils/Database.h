#pragma once

#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <string>
#include <functional>

namespace kanba {
namespace utils {

class Database {
public:
    // Get the default database client
    static drogon::orm::DbClientPtr getClient();

    // Execute a query and return results
    template<typename... Args>
    static void query(
        const std::string& sql,
        std::function<void(const drogon::orm::Result&)> callback,
        std::function<void(const drogon::orm::DrogonDbException&)> errorCallback,
        Args&&... args
    ) {
        auto client = getClient();
        if (!client) {
            LOG_ERROR << "Database client not available";
            return;
        }
        client->execSqlAsync(
            sql,
            std::forward<Args>(args)...,
            [callback](const drogon::orm::Result& result) {
                callback(result);
            },
            [errorCallback](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << "Database error: " << e.base().what();
                errorCallback(e);
            }
        );
    }

    // Execute a stored function
    static void callFunction(
        const std::string& functionName,
        const Json::Value& params,
        std::function<void(const drogon::orm::Result&)> callback,
        std::function<void(const drogon::orm::DrogonDbException&)> errorCallback
    );
};

} // namespace utils
} // namespace kanba
