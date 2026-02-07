#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "db_test_helper.h"

// Contract tests for task SQL functions.
// References: backend/src/controllers/TaskController.cpp, ProjectController.cpp

TEST_SUITE("DB Contract: Task Functions") {

TEST_CASE("create_task returns full row with expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    // TaskController.cpp:48 uses explicit casts
    const char* p[] = {
        columnId.c_str(), "My Task", "Description", "high",
        nullptr,  // assignee_id (NULL)
        nullptr,  // due_date (NULL)
        "[]",     // tags as jsonb
        userId.c_str()
    };
    PGResultGuard res(db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        8, p));

    REQUIRE(res.ntuples() == 1);

    // TaskController.cpp:61-82 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("column_id") >= 0);
    CHECK(res.col("title") >= 0);
    CHECK(res.col("description") >= 0);
    CHECK(res.col("priority") >= 0);
    CHECK(res.col("position") >= 0);
    CHECK(res.col("assignee_id") >= 0);
    CHECK(res.col("due_date") >= 0);
    CHECK(res.col("tags") >= 0);
    CHECK(res.col("created_at") >= 0);

    CHECK(res.val(0, "title") == "My Task");
    CHECK(res.val(0, "priority") == "high");
}

TEST_CASE("create_task with assignee and due_date") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    const char* p[] = {
        columnId.c_str(), "Assigned Task", "", "medium",
        userId.c_str(),           // assignee_id
        "2026-03-01T00:00:00Z",  // due_date
        "[\"bug\",\"urgent\"]",  // tags
        userId.c_str()
    };
    PGResultGuard res(db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        8, p));

    REQUIRE(res.ntuples() == 1);
    CHECK_FALSE(res.isNull(0, "assignee_id"));
    CHECK_FALSE(res.isNull(0, "due_date"));
}

TEST_CASE("get_project_tasks returns 'position' not 'task_position'") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    // Create a task first
    const char* cp[] = {
        columnId.c_str(), "Task1", "", "medium",
        nullptr, nullptr, "[]", userId.c_str()
    };
    PGResultGuard created(db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        8, cp));

    const char* p[] = { projectId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM get_project_tasks($1)", 1, p));

    REQUIRE(res.ntuples() >= 1);

    // ProjectController.cpp:165-190 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("column_id") >= 0);
    CHECK(res.col("title") >= 0);
    CHECK(res.col("description") >= 0);
    CHECK(res.col("priority") >= 0);
    CHECK(res.col("assignee_id") >= 0);
    CHECK(res.col("assignee_name") >= 0);
    CHECK(res.col("due_date") >= 0);
    CHECK(res.col("tags") >= 0);
    CHECK(res.col("created_at") >= 0);

    // CRITICAL: C++ reads row["position"], NOT row["task_position"]
    CHECK(res.col("position") >= 0);
    CHECK(res.col("task_position") < 0);
}

TEST_CASE("update_task returns full row with expected columns") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    // Create a task
    const char* cp[] = {
        columnId.c_str(), "Original", "", "low",
        nullptr, nullptr, "[]", userId.c_str()
    };
    PGResultGuard created(db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        8, cp));
    std::string taskId = created.val(0, "id");

    // TaskController.cpp:137 uses same cast pattern
    const char* p[] = {
        taskId.c_str(), "Updated", "new desc", "high",
        nullptr, nullptr, nullptr, userId.c_str()
    };
    PGResultGuard res(db.execParams(
        "SELECT * FROM update_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        8, p));

    REQUIRE(res.ntuples() == 1);

    // TaskController.cpp:150-162 reads these columns
    CHECK(res.col("id") >= 0);
    CHECK(res.col("column_id") >= 0);
    CHECK(res.col("title") >= 0);
    CHECK(res.col("description") >= 0);
    CHECK(res.col("priority") >= 0);
    CHECK(res.col("position") >= 0);
    CHECK(res.col("assignee_id") >= 0);
    CHECK(res.col("due_date") >= 0);

    CHECK(res.val(0, "title") == "Updated");
}

TEST_CASE("move_task executes without error") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);

    // Get both default columns
    const char* gp[] = { projectId.c_str() };
    PGResultGuard cols(db.execParams("SELECT * FROM get_project_columns($1)", 1, gp));
    std::string col1 = cols.val(0, "id");
    std::string col2 = cols.val(1, "id");

    // Create a task in col1
    const char* cp[] = {
        col1.c_str(), "Movable", "", "medium",
        nullptr, nullptr, "[]", userId.c_str()
    };
    PGResultGuard created(db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        8, cp));
    std::string taskId = created.val(0, "id");

    // Move to col2
    const char* p[] = { taskId.c_str(), col2.c_str(), "0", userId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM move_task($1::uuid, $2::uuid, $3, $4::uuid)", 4, p));

    CHECK(res.ntuples() == 1);
}

TEST_CASE("delete_task executes without error") {
    TestDb db; db.cleanAll();
    std::string userId = db.createTestUser();
    std::string projectId = db.createTestProject(userId);
    std::string columnId = db.getFirstColumnId(projectId);

    const char* cp[] = {
        columnId.c_str(), "Deletable", "", "low",
        nullptr, nullptr, "[]", userId.c_str()
    };
    PGResultGuard created(db.execParams(
        "SELECT * FROM create_task($1::uuid, $2, $3, $4, $5::uuid, $6::timestamptz, $7::jsonb, $8::uuid)",
        8, cp));
    std::string taskId = created.val(0, "id");

    const char* p[] = { taskId.c_str(), userId.c_str() };
    PGResultGuard res(db.execParams(
        "SELECT * FROM delete_task($1::uuid, $2::uuid)", 2, p));

    CHECK(res.ntuples() == 1);
}

} // TEST_SUITE
