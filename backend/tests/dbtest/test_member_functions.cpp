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

    auto res = db.execParams("SELECT * FROM get_project_members($1)", projectId);

    // Owner is auto-added as member by create_project
    REQUIRE(res.size() >= 1);

    // ProjectController.cpp:202-208 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "email"));
    CHECK(hasColumn(res, "role"));
    CHECK(hasColumn(res, "avatar_url"));
}

TEST_CASE("add_project_member executes without error") {
    TestDb db; db.cleanAll();
    std::string ownerId = db.createTestUser("owner@test.com", "Owner");
    std::string memberId = db.createTestUser("member@test.com", "Member");
    std::string projectId = db.createTestProject(ownerId);

    auto res = db.execParams("SELECT * FROM add_project_member($1, $2, $3)",
                              projectId, "member@test.com", "member");

    CHECK(res.size() == 1);

    // Verify member was added
    auto members = db.execParams("SELECT * FROM get_project_members($1)", projectId);
    CHECK(members.size() == 2);  // owner + new member
}

TEST_CASE("add_project_member fails for non-existent email") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    // Should raise an exception
    CHECK_THROWS(db.execParams("SELECT * FROM add_project_member($1, $2, $3)",
                                projectId, "nobody@test.com", "member"));
}

} // TEST_SUITE
