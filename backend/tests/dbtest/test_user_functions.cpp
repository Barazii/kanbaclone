#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "db_test_helper.h"

// Contract tests: verify SQL function results have the columns C++ controllers read.
// References: backend/src/controllers/AuthController.cpp

TEST_SUITE("DB Contract: User Functions") {

TEST_CASE("create_user returns columns read by AuthController") {
    TestDb db; db.cleanAll();

    auto res = db.execParams("SELECT * FROM create_user($1, $2, $3)",
                              "new@example.com", "hashedpw", "New User");

    REQUIRE(res.size() == 1);

    // AuthController.cpp:170-190 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "email"));
    CHECK(hasColumn(res, "avatar_url"));
    CHECK(hasColumn(res, "created_at"));

    // id should be a UUID (36 chars)
    CHECK(res[0]["id"].as<std::string>().length() == 36);
}

TEST_CASE("get_user_by_email returns columns read by AuthController") {
    TestDb db; db.cleanAll();
    db.createTestUser("login@test.com", "Login User");

    auto res = db.execParams("SELECT * FROM get_user_by_email($1)",
                              "login@test.com");

    REQUIRE(res.size() >= 1);

    // AuthController.cpp:70-73 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "email"));
    CHECK(hasColumn(res, "password_hash"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "avatar_url"));
    CHECK(hasColumn(res, "created_at"));
}

TEST_CASE("get_user_by_email returns empty for non-existent user") {
    TestDb db; db.cleanAll();

    auto res = db.execParams("SELECT * FROM get_user_by_email($1)",
                              "nobody@test.com");

    CHECK(res.size() == 0);
}

TEST_CASE("get_user_by_id returns columns read by AuthController") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();

    auto res = db.execParams("SELECT * FROM get_user_by_id($1)", userId);

    REQUIRE(res.size() == 1);

    // AuthController.cpp:272-278 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "email"));
    CHECK(hasColumn(res, "avatar_url"));
    CHECK(hasColumn(res, "created_at"));
}

} // TEST_SUITE
