#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "db_test_helper.h"

// Contract tests for task SQL functions.
// References: backend/src/controllers/TaskController.cpp, ProjectController.cpp

using null = std::optional<std::string>;

TEST_SUITE("DB Contract: Task Functions") {

TEST_CASE("create_task returns full row with expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    // TaskController.cpp:48 uses explicit casts
    auto res = db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        columnId, "My Task", "Description", "high",
        null{}, null{}, "[]", userId);

    REQUIRE(res.size() == 1);

    // TaskController.cpp:61-82 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "column_id"));
    CHECK(hasColumn(res, "title"));
    CHECK(hasColumn(res, "description"));
    CHECK(hasColumn(res, "priority"));
    CHECK(hasColumn(res, "position"));
    CHECK(hasColumn(res, "assignee_id"));
    CHECK(hasColumn(res, "due_date"));
    CHECK(hasColumn(res, "tags"));
    CHECK(hasColumn(res, "created_at"));

    CHECK(res[0]["title"].as<std::string>() == "My Task");
    CHECK(res[0]["priority"].as<std::string>() == "high");
}

TEST_CASE("create_task with assignee and due_date") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    auto res = db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        columnId, "Assigned Task", "", "medium",
        userId, "2026-03-01T00:00:00Z", "[\"bug\",\"urgent\"]", userId);

    REQUIRE(res.size() == 1);
    CHECK_FALSE(res[0]["assignee_id"].is_null());
    CHECK_FALSE(res[0]["due_date"].is_null());
}

TEST_CASE("get_project_tasks returns 'position' not 'task_position'") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    // Create a task first
    db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        columnId, "Task1", "", "medium",
        null{}, null{}, "[]", userId);

    auto res = db.execParams("SELECT * FROM get_project_tasks($1)", projectId);

    REQUIRE(res.size() >= 1);

    // ProjectController.cpp:165-190 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "column_id"));
    CHECK(hasColumn(res, "title"));
    CHECK(hasColumn(res, "description"));
    CHECK(hasColumn(res, "priority"));
    CHECK(hasColumn(res, "assignee_id"));
    CHECK(hasColumn(res, "assignee_name"));
    CHECK(hasColumn(res, "due_date"));
    CHECK(hasColumn(res, "tags"));
    CHECK(hasColumn(res, "created_at"));

    // CRITICAL: C++ reads row["position"], NOT row["task_position"]
    CHECK(hasColumn(res, "position"));
    CHECK_FALSE(hasColumn(res, "task_position"));
}

TEST_CASE("update_task returns full row with expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    // Create a task
    auto created = db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        columnId, "Original", "", "low",
        null{}, null{}, "[]", userId);
    std::string taskId = created[0]["id"].as<std::string>();

    // TaskController.cpp:137 uses same cast pattern
    auto res = db.execParams(
        "SELECT * FROM update_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        taskId, "Updated", "new desc", "high",
        null{}, null{}, null{}, userId);

    REQUIRE(res.size() == 1);

    // TaskController.cpp:150-162 reads these columns
    CHECK(hasColumn(res, "id"));
    CHECK(hasColumn(res, "column_id"));
    CHECK(hasColumn(res, "title"));
    CHECK(hasColumn(res, "description"));
    CHECK(hasColumn(res, "priority"));
    CHECK(hasColumn(res, "position"));
    CHECK(hasColumn(res, "assignee_id"));
    CHECK(hasColumn(res, "due_date"));

    CHECK(res[0]["title"].as<std::string>() == "Updated");
}

TEST_CASE("move_task executes without error") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    // Get both default columns
    auto cols = db.execParams("SELECT * FROM get_project_columns($1)", projectId);
    std::string col1 = cols[0]["id"].as<std::string>();
    std::string col2 = cols[1]["id"].as<std::string>();

    // Create a task in col1
    auto created = db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        col1, "Movable", "", "medium",
        null{}, null{}, "[]", userId);
    std::string taskId = created[0]["id"].as<std::string>();

    // Move to col2
    auto res = db.execParams(
        "SELECT * FROM move_task($1::uuid, $2::uuid, $3, $4::uuid)",
        taskId, col2, "0", userId);

    CHECK(res.size() == 1);
}

TEST_CASE("delete_task executes without error") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    auto created = db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        columnId, "Deletable", "", "low",
        null{}, null{}, "[]", userId);
    std::string taskId = created[0]["id"].as<std::string>();

    auto res = db.execParams("SELECT * FROM delete_task($1::uuid, $2::uuid)",
                              taskId, userId);

    CHECK(res.size() == 1);
}

} // TEST_SUITE
