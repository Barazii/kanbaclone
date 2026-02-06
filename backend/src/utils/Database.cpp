#include "Database.h"

namespace kanba {
namespace utils {

drogon::orm::DbClientPtr Database::getClient() {
    return drogon::app().getDbClient("default");
}

void Database::callFunction(
    const std::string& functionName,
    const Json::Value& params,
    std::function<void(const drogon::orm::Result&)> callback,
    std::function<void(const drogon::orm::DrogonDbException&)> errorCallback
) {
    auto client = getClient();
    if (!client) {
        LOG_ERROR << "Database client not available";
        return;
    }

    // Build the SQL call for the function
    std::string sql = "SELECT * FROM " + functionName + "(";
    std::vector<std::string> paramPlaceholders;

    int paramIndex = 1;
    for (size_t i = 0; i < params.getMemberNames().size(); ++i) {
        paramPlaceholders.push_back("$" + std::to_string(paramIndex++));
    }

    for (size_t i = 0; i < paramPlaceholders.size(); ++i) {
        if (i > 0) sql += ", ";
        sql += paramPlaceholders[i];
    }
    sql += ")";

    LOG_DEBUG << "Calling function: " << sql;

    // Execute with parameters
    client->execSqlAsync(
        sql,
        [callback](const drogon::orm::Result& result) {
            callback(result);
        },
        [errorCallback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Database error: " << e.base().what();
            errorCallback(e);
        }
    );
}

} // namespace utils
} // namespace kanba
