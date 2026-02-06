#pragma once

#include <string>

namespace kanba {
namespace utils {

class PasswordHash {
public:
    // Hash a password using Argon2id (libsodium)
    static std::string hash(const std::string& password);

    // Verify a password against a hash
    // Supports both Argon2id (new) and bcrypt (legacy) hashes
    static bool verify(const std::string& password, const std::string& hash);

    // Check if a hash is bcrypt format (for migration)
    static bool isBcryptHash(const std::string& hash);

    // Initialize libsodium (call once at startup)
    static bool initialize();
};

} // namespace utils
} // namespace kanba
