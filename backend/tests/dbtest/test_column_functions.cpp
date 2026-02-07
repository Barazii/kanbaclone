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

    const char* p[] = { projectId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM get_project_columns($1)", 1, p));

    REQUIRE(res.ntuples() >= 2);

    // ProjectController.cpp:147-153 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("color") >= 0);
    CHECK(res.col("task_count") >= 0);

    // CRITICAL: C++ reads row["position"], NOT row["col_position"]
    CHECK(res.col("position") >= 0);
    CHECK(res.col("col_position") < 0);  // must NOT exist
}

TEST_CASE("create_column returns full row with expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    const char* p[] = { projectId.c_str(), "In Progress", "#f59e0b" };
    PGResultGuard res(db.execParams(
        "SELECT * FROM create_column($1, $2, $3)", 3, p));

    REQUIRE(res.ntuples() == 1);

    // ColumnController.cpp:41-47 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("project_id") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("color") >= 0);
    CHECK(res.col("position") >= 0);

    CHECK(res.val(0, "id").length() == 36);
    CHECK(res.val(0, "name") == "In Progress");
}

TEST_CASE("update_column returns full row with expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    const char* p[] = { columnId.c_str(), "Renamed", "#ff0000" };
    PGResultGuard res(db.execParams(
        "SELECT * FROM update_column($1, $2, $3)", 3, p));

    REQUIRE(res.ntuples() == 1);

    // ColumnController.cpp:99-104 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("name") >= 0);
    CHECK(res.col("color") >= 0);
    CHECK(res.col("position") >= 0);

    CHECK(res.val(0, "name") == "Renamed");
}

TEST_CASE("delete_column executes without error") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    // Create a 3rd column to delete (keep the 2 defaults)
    const char* cp[] = { projectId.c_str(), "Temp", "#000" };
    PGResultGuard created(db.execParams(
        "SELECT * FROM create_column($1, $2, $3)", 3, cp));
    std::string colId = created.val(0, "id");

    const char* p[] = { colId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM delete_column($1)", 1, p));

    CHECK(res.ntuples() == 1);
}

} // TEST_SUITE
