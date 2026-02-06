#include "Session.h"
#include "Database.h"
#include <uuid/uuid.h>

namespace kanba {
namespace utils {

std::string Session::generateSessionId() {
    uuid_t uuid;
    char uuidStr[37];

    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuidStr);

    return std::string(uuidStr);
}

void Session::createSession(
    const std::string& sessionId,
    const std::string& userId,
    std::function<void(bool success)> callback
) {
    auto db = Database::getClient();
    if (!db) {
        callback(false);
        return;
    }

    db->execSqlAsync(
        "INSERT INTO sessions (id, user_id, expires_at) VALUES ($1, $2, NOW() + INTERVAL '7 days') "
        "ON CONFLICT (id) DO UPDATE SET user_id = $2, expires_at = NOW() + INTERVAL '7 days'",
        [callback](const drogon::orm::Result& result) {
            callback(true);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Failed to create session: " << e.base().what();
            callback(false);
        },
        sessionId,
        userId
    );
}

void Session::getUserIdFromSession(
    const std::string& sessionId,
    std::function<void(std::optional<std::string> userId)> callback
) {
    auto db = Database::getClient();
    if (!db) {
        callback(std::nullopt);
        return;
    }

    db->execSqlAsync(
        "SELECT user_id FROM sessions WHERE id = $1 AND expires_at > NOW()",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                callback(std::nullopt);
            } else {
                callback(result[0]["user_id"].as<std::string>());
            }
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Failed to get session: " << e.base().what();
            callback(std::nullopt);
        },
        sessionId
    );
}

void Session::deleteSession(
    const std::string& sessionId,
    std::function<void(bool success)> callback
) {
    auto db = Database::getClient();
    if (!db) {
        callback(false);
        return;
    }

    db->execSqlAsync(
        "DELETE FROM sessions WHERE id = $1",
        [callback](const drogon::orm::Result& result) {
            callback(true);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Failed to delete session: " << e.base().what();
            callback(false);
        },
        sessionId
    );
}

void Session::isValidSession(
    const std::string& sessionId,
    std::function<void(bool valid)> callback
) {
    getUserIdFromSession(sessionId, [callback](std::optional<std::string> userId) {
        callback(userId.has_value());
    });
}

void Session::cleanupExpiredSessions(
    std::function<void(int deletedCount)> callback
) {
    auto db = Database::getClient();
    if (!db) {
        callback(0);
        return;
    }

    db->execSqlAsync(
        "DELETE FROM sessions WHERE expires_at < NOW()",
        [callback](const drogon::orm::Result& result) {
            callback(static_cast<int>(result.affectedRows()));
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Failed to cleanup sessions: " << e.base().what();
            callback(0);
        }
    );
}

} // namespace utils
} // namespace kanba
