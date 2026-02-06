#pragma once

#include <drogon/drogon.h>
#include <string>
#include <optional>

namespace kanba {
namespace utils {

class Session {
public:
    // Generate a new session ID (UUID)
    static std::string generateSessionId();

    // Create a new session in the database
    static void createSession(
        const std::string& sessionId,
        const std::string& userId,
        std::function<void(bool success)> callback
    );

    // Get user ID from session
    static void getUserIdFromSession(
        const std::string& sessionId,
        std::function<void(std::optional<std::string> userId)> callback
    );

    // Delete a session
    static void deleteSession(
        const std::string& sessionId,
        std::function<void(bool success)> callback
    );

    // Check if session exists and is valid
    static void isValidSession(
        const std::string& sessionId,
        std::function<void(bool valid)> callback
    );

    // Clean up expired sessions
    static void cleanupExpiredSessions(
        std::function<void(int deletedCount)> callback
    );

    // Session cookie configuration
    static constexpr const char* COOKIE_NAME = "session";
    static constexpr int SESSION_TTL_SECONDS = 7 * 24 * 60 * 60; // 7 days
};

} // namespace utils
} // namespace kanba
