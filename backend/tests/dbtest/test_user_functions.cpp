#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "db_test_helper.h"

// Contract tests: verify SQL function results have the columns C++ controllers read.
// References: backend/src/controllers/AuthController.cpp

TEST_SUITE("DB Contract: User Functions") {

TEST_CASE("create_user returns columns read by AuthController") {
    TestDb db; db.cleanAll();

    const char* p[] = { "new@example.com", "hashedpw", "New User" };
    PGResultGuard res(db.execParams("SELECT * FROM create_user($1, $2, $3)", 3, p));

    REQUIRE(res.ntuples() == 1);

    // AuthController.cpp:170-190 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("email") >= 0);
    CHECK(res.col("avatar_url") >= 0);
    CHECK(res.col("created_at") >= 0);

    // id should be a UUID (36 chars)
    CHECK(res.val(0, "id").length() == 36);
}

TEST_CASE("get_user_by_email returns columns read by AuthController") {
    TestDb db; db.cleanAll();
    db.createTestUser("login@test.com", "Login User");

    const char* p[] = { "login@test.com" };
    PGResultGuard res(db.execParams("SELECT * FROM get_user_by_email($1)", 1, p));

    REQUIRE(res.ntuples() >= 1);

    // AuthController.cpp:70-73 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("email") >= 0);
    CHECK(res.col("password_hash") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("avatar_url") >= 0);
    CHECK(res.col("created_at") >= 0);
}

TEST_CASE("get_user_by_email returns empty for non-existent user") {
    TestDb db; db.cleanAll();

    const char* p[] = { "nobody@test.com" };
    PGResultGuard res(db.execParams("SELECT * FROM get_user_by_email($1)", 1, p));

    CHECK(res.ntuples() == 0);
}

TEST_CASE("get_user_by_id returns columns read by AuthController") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();

    const char* p[] = { userId.c_str() };
    PGResultGuard res(db.execParams("SELECT * FROM get_user_by_id($1)", 1, p));

    REQUIRE(res.ntuples() == 1);

    // AuthController.cpp:272-278 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("email") >= 0);
    CHECK(res.col("avatar_url") >= 0);
    CHECK(res.col("created_at") >= 0);
}

} // TEST_SUITE
