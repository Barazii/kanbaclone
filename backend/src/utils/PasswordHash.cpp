#include "PasswordHash.h"
#include <sodium.h>
#include <cstring>
#include <stdexcept>

namespace kanba {
namespace utils {

bool PasswordHash::initialize() {
    if (sodium_init() < 0) {
        return false;
    }
    return true;
}

std::string PasswordHash::hash(const std::string& password) {
    char hashed[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
        hashed,
        password.c_str(),
        password.length(),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE
    ) != 0) {
        throw std::runtime_error("Password hashing failed (out of memory)");
    }

    return std::string(hashed);
}

bool PasswordHash::verify(const std::string& password, const std::string& storedHash) {
    // Check if it's a bcrypt hash (starts with $2a$, $2b$, or $2y$)
    if (isBcryptHash(storedHash)) {
        // For bcrypt compatibility, we need to use a bcrypt library
        // Since we're using libsodium, we'll need to handle migration
        // For now, return false for bcrypt hashes (require password reset)
        // In production, you'd want to include a bcrypt library for verification

        // TODO: Implement bcrypt verification for migration
        // This would require linking against libbcrypt or similar
        return false;
    }

    // Verify using Argon2id (libsodium)
    return crypto_pwhash_str_verify(
        storedHash.c_str(),
        password.c_str(),
        password.length()
    ) == 0;
}

bool PasswordHash::isBcryptHash(const std::string& hash) {
    return hash.length() >= 4 && (
        hash.substr(0, 4) == "$2a$" ||
        hash.substr(0, 4) == "$2b$" ||
        hash.substr(0, 4) == "$2y$"
    );
}

} // namespace utils
} // namespace kanba
