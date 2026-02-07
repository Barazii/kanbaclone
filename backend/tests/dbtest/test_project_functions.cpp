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
    const char* p[] = { "My Project", "desc", "", userId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT create_project($1, $2, $3, $4) AS id", 4, p));

    REQUIRE(res.ntuples() == 1);
    CHECK(res.col("id") >= 0);
    CHECK(res.val(0, "id").length() == 36);
}

TEST_CASE("create_project via SELECT * returns 'create_project' NOT 'id'") {
    // Documents why the AS id alias is required
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser("doc@test.com", "Doc");

    const char* p[] = { "Project2", "desc", "", userId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM create_project($1, $2, $3, $4)", 4, p));

    REQUIRE(res.ntuples() == 1);
    CHECK(res.col("create_project") >= 0);
    CHECK(res.col("id") < 0);  // "id" does NOT exist without alias
}

TEST_CASE("get_user_projects returns columns read by ProjectController") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    db.createTestProject(userId);

    const char* p[] = { userId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM get_user_projects($1)", 1, p));

    REQUIRE(res.ntuples() >= 1);

    // ProjectController.cpp:22-33 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("description") >= 0);
    CHECK(res.col("icon") >= 0);
    CHECK(res.col("owner_id") >= 0);
    CHECK(res.col("task_count") >= 0);
    CHECK(res.col("member_count") >= 0);
    CHECK(res.col("created_at") >= 0);
}

TEST_CASE("get_project_details returns columns read by ProjectController") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    const char* p[] = { projectId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM get_project_details($1)", 1, p));

    REQUIRE(res.ntuples() == 1);

    // ProjectController.cpp:129-138 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("description") >= 0);
    CHECK(res.col("icon") >= 0);
    CHECK(res.col("owner_id") >= 0);
    CHECK(res.col("created_at") >= 0);
}

TEST_CASE("delete_project executes without error") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    const char* p[] = { projectId.c_str(), userId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM delete_project($1, $2)", 2, p));

    // Returns boolean scalar - C++ doesn't read columns, just checks success
    CHECK(res.ntuples() == 1);
}

} // TEST_SUITE
