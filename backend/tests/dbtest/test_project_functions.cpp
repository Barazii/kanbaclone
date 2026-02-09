#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "db_test_helper.h"

// Contract tests for project SQL functions.
// References: backend/src/controllers/ProjectController.cpp

TEST_SUITE("DB Contract: Project Functions") {

TEST_CASE("create_project with AS id alias returns 'id' column") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();

    // ProjectController.cpp:72 uses: SELECT create_project($1,$2,$3,$4) AS id
    auto res = db.execParams(
        "SELECT create_project($1, $2, $3, $4) AS id",
        "My Project", "desc", "", userId);

    REQUIRE(res.size() == 1);
    CHECK(hasColumn(res, "id"));
    CHECK(res[0]["id"].as<std::string>().length() == 36);
}

TEST_CASE("create_project via SELECT * returns 'create_project' NOT 'id'") {
    // Documents why the AS id alias is required
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser("doc@test.com", "Doc");

    auto res = db.execParams(
        "SELECT * FROM create_project($1, $2, $3, $4)",
        "Project2", "desc", "", userId);

    REQUIRE(res.size() == 1);
    CHECK(hasColumn(res, "create_project"));
    CHECK_FALSE(hasColumn(res, "id"));  // "id" does NOT exist without alias
}

TEST_CASE("get_user_projects returns columns read by ProjectController") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    db.createTestProject(userId);

    auto res = db.execParams("SELECT * FROM get_user_projects($1)", userId);

    REQUIRE(res.size() >= 1);

    // ProjectController.cpp:22-33 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "description"));
    CHECK(hasColumn(res, "icon"));
    CHECK(hasColumn(res, "owner_id"));
    CHECK(hasColumn(res, "task_count"));
    CHECK(hasColumn(res, "member_count"));
    CHECK(hasColumn(res, "created_at"));
}

TEST_CASE("get_project_details returns columns read by ProjectController") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    auto res = db.execParams("SELECT * FROM get_project_details($1)", projectId);

    REQUIRE(res.size() == 1);

    // ProjectController.cpp:129-138 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "description"));
    CHECK(hasColumn(res, "icon"));
    CHECK(hasColumn(res, "owner_id"));
    CHECK(hasColumn(res, "created_at"));
}

TEST_CASE("delete_project executes without error") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    auto res = db.execParams("SELECT * FROM delete_project($1, $2)",
                              projectId, userId);

    // Returns boolean scalar - C++ doesn't read columns, just checks success
    CHECK(res.size() == 1);
}

} // TEST_SUITE
