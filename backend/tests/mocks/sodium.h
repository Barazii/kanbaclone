#pragma once
// =============================================================================
// Minimal libsodium stub for testing PasswordHash
// Provides crypto_pwhash_str / crypto_pwhash_str_verify stubs that use
// simple (non-cryptographic) hashing so tests run fast and deterministically.
// =============================================================================

#include <cstring>
#include <string>
#include <functional>

#define crypto_pwhash_STRBYTES 128
#define crypto_pwhash_OPSLIMIT_INTERACTIVE 2
#define crypto_pwhash_MEMLIMIT_INTERACTIVE 67108864

// Tracks initialization state
inline bool& sodium_initialized() {
    static bool init = false;
    return init;
}

inline int sodium_init() {
    sodium_initialized() = true;
    return 0;  // success
}

// Simple test hash: "$argon2id$test$" + hash_of_password
// Uses a trivial hash so we can verify in tests
inline std::string simple_hash(const char* passwd, size_t passwdlen) {
    // Simple FNV-1a-like hash for testing
    size_t hash = 2166136261u;
    for (size_t i = 0; i < passwdlen; i++) {
        hash ^= static_cast<size_t>(passwd[i]);
        hash *= 16777619u;
    }
    return "$argon2id$test$" + std::to_string(hash);
}

inline int crypto_pwhash_str(
    char out[crypto_pwhash_STRBYTES],
    const char* passwd,
    unsigned long long passwdlen,
    unsigned long long /*opslimit*/,
    size_t /*memlimit*/
) {
    std::string h = simple_hash(passwd, static_cast<size_t>(passwdlen));
    std::strncpy(out, h.c_str(), crypto_pwhash_STRBYTES - 1);
    out[crypto_pwhash_STRBYTES - 1] = '\0';
    return 0;  // success
}

inline int crypto_pwhash_str_verify(
    const char* str,
    const char* passwd,
    unsigned long long passwdlen
) {
    std::string expected = simple_hash(passwd, static_cast<size_t>(passwdlen));
    return (std::string(str) == expected) ? 0 : -1;
}
