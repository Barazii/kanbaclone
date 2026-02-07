#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "db_test_helper.h"

// Contract tests for project member SQL functions.
// References: backend/src/controllers/ProjectController.cpp

TEST_SUITE("DB Contract: Member Functions") {

TEST_CASE("get_project_members returns expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    const char* p[] = { projectId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM get_project_members($1)", 1, p));

    // Owner is auto-added as member by create_project
    REQUIRE(res.ntuples() >= 1);

    // ProjectController.cpp:202-208 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("email") >= 0);
    CHECK(res.col("role") >= 0);
    CHECK(res.col("avatar_url") >= 0);
}

TEST_CASE("add_project_member executes without error") {
    TestDb db; db.cleanAll();
    std::string ownerId = db.createTestUser("owner@test.com", "Owner");
    std::string memberId = db.createTestUser("member@test.com", "Member");
    std::string projectId = db.createTestProject(ownerId);

    const char* p[] = { projectId.c_str(), "member@test.com", "member" };
    PGResultGuard res(db.execParams(
        "SELECT * FROM add_project_member($1, $2, $3)", 3, p));

    CHECK(res.ntuples() == 1);

    // Verify member was added
    const char* mp[] = { projectId.c_str() };
    PGResultGuard members(db.execParams(
        "SELECT * FROM get_project_members($1)", 1, mp));
    CHECK(members.ntuples() == 2);  // owner + new member
}

TEST_CASE("add_project_member fails for non-existent email") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    const char* p[] = { projectId.c_str(), "nobody@test.com", "member" };
    // Should raise an exception
    CHECK_THROWS(db.execParams(
        "SELECT * FROM add_project_member($1, $2, $3)", 3, p));
}

} // TEST_SUITE
