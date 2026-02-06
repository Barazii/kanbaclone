// =============================================================================
// Unit Tests: PasswordHash
//
// Tests bcrypt detection logic and migration path behavior.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "utils/PasswordHash.h"

using kanba::utils::PasswordHash;

TEST_SUITE("PasswordHash") {

// ---------------------------------------------------------------------------
// Bcrypt detection
// ---------------------------------------------------------------------------

TEST_CASE("isBcryptHash should return true for $2a$ prefix") {
    CHECK(PasswordHash::isBcryptHash("$2a$10$abcdefghijklmnopqrstuv") == true);
}

TEST_CASE("isBcryptHash should return true for $2b$ prefix") {
    CHECK(PasswordHash::isBcryptHash("$2b$12$abcdefghijklmnopqrstuv") == true);
}

TEST_CASE("isBcryptHash should return true for $2y$ prefix") {
    CHECK(PasswordHash::isBcryptHash("$2y$10$abcdefghijklmnopqrstuv") == true);
}

TEST_CASE("isBcryptHash should return false for argon2id hash") {
    CHECK(PasswordHash::isBcryptHash("$argon2id$v=19$m=65536,t=2,p=1$...") == false);
}

TEST_CASE("isBcryptHash should return false for empty string") {
    CHECK(PasswordHash::isBcryptHash("") == false);
}

TEST_CASE("isBcryptHash should return false for short string") {
    CHECK(PasswordHash::isBcryptHash("$2") == false);
    CHECK(PasswordHash::isBcryptHash("$2a") == false);
}

TEST_CASE("isBcryptHash should return false for random text") {
    CHECK(PasswordHash::isBcryptHash("notahash") == false);
}

TEST_CASE("verify should return false for bcrypt hashes (migration path)") {
    // The implementation intentionally returns false for bcrypt hashes
    // to force password reset during migration.
    PasswordHash::initialize();
    CHECK(PasswordHash::verify("password", "$2a$10$abcdefghijklmnopqrstuv") == false);
    CHECK(PasswordHash::verify("password", "$2b$12$abcdefghijklmnopqrstuv") == false);
    CHECK(PasswordHash::verify("password", "$2y$10$abcdefghijklmnopqrstuv") == false);
}

} // TEST_SUITE
