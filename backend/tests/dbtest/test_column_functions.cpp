#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "db_test_helper.h"

// Contract tests for column SQL functions.
// References: backend/src/controllers/ColumnController.cpp, ProjectController.cpp

TEST_SUITE("DB Contract: Column Functions") {

TEST_CASE("get_project_columns returns 'position' not 'col_position'") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    // create_project creates 2 default columns

    auto res = db.execParams("SELECT * FROM get_project_columns($1)", projectId);

    REQUIRE(res.size() >= 2);

    // ProjectController.cpp:147-153 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "color"));
    CHECK(hasColumn(res, "task_count"));

    // CRITICAL: C++ reads row["position"], NOT row["col_position"]
    CHECK(hasColumn(res, "position"));
    CHECK_FALSE(hasColumn(res, "col_position"));  // must NOT exist
}

TEST_CASE("create_column returns full row with expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    auto res = db.execParams("SELECT * FROM create_column($1, $2, $3)",
                              projectId, "In Progress", "#f59e0b");

    REQUIRE(res.size() == 1);

    // ColumnController.cpp:41-47 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "project_id"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "color"));
    CHECK(hasColumn(res, "position"));

    CHECK(res[0]["id"].as<std::string>().length() == 36);
    CHECK(res[0]["name"].as<std::string>() == "In Progress");
}

TEST_CASE("update_column returns full row with expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    auto res = db.execParams("SELECT * FROM update_column($1, $2, $3)",
                              columnId, "Renamed", "#ff0000");

    REQUIRE(res.size() == 1);

    // ColumnController.cpp:99-104 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "name"));
    CHECK(hasColumn(res, "color"));
    CHECK(hasColumn(res, "position"));

    CHECK(res[0]["name"].as<std::string>() == "Renamed");
}

TEST_CASE("delete_column executes without error") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    // Create a 3rd column to delete (keep the 2 defaults)
    auto created = db.execParams("SELECT * FROM create_column($1, $2, $3)",
                                  projectId, "Temp", "#000");
    std::string colId = created[0]["id"].as<std::string>();

    auto res = db.execParams("SELECT * FROM delete_column($1)", colId);

    CHECK(res.size() == 1);
}

} // TEST_SUITE
