// =============================================================================
// Integration Tests: Project Workflow
//
// Tests the full project lifecycle:
//   create project -> create columns -> create tasks -> move tasks ->
//   update tasks -> delete tasks -> delete columns -> delete project
// All using controllers with mock database.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "controllers/ProjectController.h"
#include "controllers/ColumnController.h"
#include "controllers/TaskController.h"
#include "filters/AuthFilter.h"
#include "utils/Session.h"
#include "utils/Database.h"

using namespace kanba::controllers;
using namespace kanba::filters;
using namespace drogon;
using namespace drogon::orm;

static std::shared_ptr<DbClient> gDb;

static void setupDb() {
    gDb = std::make_shared<DbClient>();
    drogon::app().setDbClient("default", gDb);
}

static HttpRequestPtr makeAuthJsonReq(const Json::Value& json, const std::string& userId = "user-1") {
    auto req = HttpRequest::newHttpRequest();
    req->setJsonBody(json);
    req->attributes()->insert<std::string>("userId", userId);
    return req;
}

static HttpRequestPtr makeAuthReq(const std::string& userId = "user-1") {
    auto req = HttpRequest::newHttpRequest();
    req->attributes()->insert<std::string>("userId", userId);
    return req;
}

TEST_SUITE("Integration: Project Workflow") {

// ---------------------------------------------------------------------------
// Step 1: Create a project
// ---------------------------------------------------------------------------

TEST_CASE("should create a new project") {
    setupDb();

    Result r;
    Row row;
    row.addField("id", "proj-001");
    r.addRow(row);
    gDb->setNextResult(r);

    ProjectController projCtrl;
    Json::Value body;
    body["name"] = "Kanban Board";
    body["description"] = "My task management project";
    body["icon"] = "clipboard";
    auto req = makeAuthJsonReq(body);

    HttpResponsePtr resp;
    projCtrl.createProject(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k201Created);
    CHECK((*resp->getJsonObject())["id"].asString() == "proj-001");
    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

// ---------------------------------------------------------------------------
// Step 2: List projects
// ---------------------------------------------------------------------------

TEST_CASE("should list user projects after creation") {
    setupDb();

    Result r;
    Row row;
    row.addField("id", "proj-001");
    row.addField("name", "Kanban Board");
    row.addField("description", "My task management project");
    row.addField("icon", "clipboard");
    row.addField("owner_id", "user-1");
    row.addField("task_count", "0");
    row.addField("member_count", "1");
    row.addField("created_at", "2025-01-01T00:00:00Z");
    r.addRow(row);
    gDb->setNextResult(r);

    ProjectController projCtrl;
    auto req = makeAuthReq();

    HttpResponsePtr resp;
    projCtrl.getProjects(req, [&](const HttpResponsePtr& r) { resp = r; });

    auto json = resp->getJsonObject();
    auto projects = (*json)["projects"];
    REQUIRE(projects.isArray());
    CHECK(projects.size() == 1);
    CHECK(projects[0]["name"].asString() == "Kanban Board");
}

// ---------------------------------------------------------------------------
// Step 3: Create columns in the project
// ---------------------------------------------------------------------------

TEST_CASE("should create columns: To Do, In Progress, Done") {
    setupDb();
    ColumnController colCtrl;

    // Column 1: To Do
    {
        Result r;
        Row row;
        row.addField("id", "col-todo");
        row.addField("project_id", "proj-001");
        row.addField("name", "To Do");
        row.addField("color", "#3b82f6");
        row.addField("position", "0");
        r.addRow(row);
        gDb->setNextResult(r);

        Json::Value body;
        body["project_id"] = "proj-001";
        body["name"] = "To Do";
        body["color"] = "#3b82f6";
        auto req = makeAuthJsonReq(body);

        HttpResponsePtr resp;
        colCtrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

        CHECK(resp->statusCode() == k201Created);
        CHECK((*resp->getJsonObject())["id"].asString() == "col-todo");
        CHECK((*resp->getJsonObject())["position"].asInt() == 0);
    }

    // Column 2: In Progress
    {
        Result r;
        Row row;
        row.addField("id", "col-progress");
        row.addField("project_id", "proj-001");
        row.addField("name", "In Progress");
        row.addField("color", "#f59e0b");
        row.addField("position", "1");
        r.addRow(row);
        gDb->setNextResult(r);

        Json::Value body;
        body["project_id"] = "proj-001";
        body["name"] = "In Progress";
        body["color"] = "#f59e0b";
        auto req = makeAuthJsonReq(body);

        HttpResponsePtr resp;
        colCtrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

        CHECK(resp->statusCode() == k201Created);
        CHECK((*resp->getJsonObject())["position"].asInt() == 1);
    }

    // Column 3: Done
    {
        Result r;
        Row row;
        row.addField("id", "col-done");
        row.addField("project_id", "proj-001");
        row.addField("name", "Done");
        row.addField("color", "#22c55e");
        row.addField("position", "2");
        r.addRow(row);
        gDb->setNextResult(r);

        Json::Value body;
        body["project_id"] = "proj-001";
        body["name"] = "Done";
        body["color"] = "#22c55e";
        auto req = makeAuthJsonReq(body);

        HttpResponsePtr resp;
        colCtrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

        CHECK(resp->statusCode() == k201Created);
        CHECK((*resp->getJsonObject())["name"].asString() == "Done");
    }
}

// ---------------------------------------------------------------------------
// Step 4: Create tasks in columns
// ---------------------------------------------------------------------------

TEST_CASE("should create tasks in the To Do column") {
    setupDb();
    TaskController taskCtrl;

    Result r;
    Row row;
    row.addField("id", "task-001");
    row.addField("column_id", "col-todo");
    row.addField("title", "Write unit tests");
    row.addField("description", "Cover all controllers");
    row.addField("priority", "high");
    row.addField("position", "0");
    row.addField("assignee_id", "", true);
    row.addField("due_date", "", true);
    row.addField("tags", "", true);
    row.addField("created_at", "2025-01-01T00:00:00Z");
    r.addRow(row);
    gDb->setNextResult(r);

    Json::Value body;
    body["column_id"] = "col-todo";
    body["title"] = "Write unit tests";
    body["description"] = "Cover all controllers";
    body["priority"] = "high";
    auto req = makeAuthJsonReq(body);

    HttpResponsePtr resp;
    taskCtrl.createTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k201Created);
    auto json = resp->getJsonObject();
    CHECK((*json)["id"].asString() == "task-001");
    CHECK((*json)["title"].asString() == "Write unit tests");
    CHECK((*json)["priority"].asString() == "high");
}

// ---------------------------------------------------------------------------
// Step 5: Move a task between columns
// ---------------------------------------------------------------------------

TEST_CASE("should move a task from To Do to In Progress") {
    setupDb();
    gDb->setNextResult(Result{});

    TaskController taskCtrl;
    Json::Value body;
    body["task_id"] = "task-001";
    body["column_id"] = "col-progress";
    body["position"] = 0;
    auto req = makeAuthJsonReq(body);

    HttpResponsePtr resp;
    taskCtrl.moveTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
    CHECK(gDb->lastSql().find("move_task") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Step 6: Update a task
// ---------------------------------------------------------------------------

TEST_CASE("should update task title and priority") {
    setupDb();

    Result r;
    Row row;
    row.addField("id", "task-001");
    row.addField("column_id", "col-progress");
    row.addField("title", "Updated: Write integration tests");
    row.addField("description", "Cover all controllers");
    row.addField("priority", "critical");
    row.addField("position", "0");
    row.addField("assignee_id", "user-1");
    row.addField("due_date", "", true);
    r.addRow(row);
    gDb->setNextResult(r);

    TaskController taskCtrl;
    Json::Value body;
    body["id"] = "task-001";
    body["title"] = "Updated: Write integration tests";
    body["priority"] = "critical";
    body["assignee_id"] = "user-1";
    auto req = makeAuthJsonReq(body);

    HttpResponsePtr resp;
    taskCtrl.updateTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k200OK);
    auto json = resp->getJsonObject();
    CHECK((*json)["title"].asString() == "Updated: Write integration tests");
    CHECK((*json)["priority"].asString() == "critical");
    CHECK((*json)["assignee_id"].asString() == "user-1");
}

// ---------------------------------------------------------------------------
// Step 7: Update a column
// ---------------------------------------------------------------------------

TEST_CASE("should rename a column") {
    setupDb();

    Result r;
    Row row;
    row.addField("id", "col-progress");
    row.addField("name", "Working On It");
    row.addField("color", "#f59e0b");
    row.addField("position", "1");
    r.addRow(row);
    gDb->setNextResult(r);

    ColumnController colCtrl;
    Json::Value body;
    body["id"] = "col-progress";
    body["name"] = "Working On It";
    auto req = makeAuthJsonReq(body);

    HttpResponsePtr resp;
    colCtrl.updateColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k200OK);
    CHECK((*resp->getJsonObject())["name"].asString() == "Working On It");
}

// ---------------------------------------------------------------------------
// Step 8: Invite a member to the project
// ---------------------------------------------------------------------------

TEST_CASE("should invite a member to the project") {
    setupDb();
    gDb->setNextResult(Result{});

    ProjectController projCtrl;
    Json::Value body;
    body["email"] = "colleague@example.com";
    body["role"] = "member";
    auto req = makeAuthJsonReq(body);

    HttpResponsePtr resp;
    projCtrl.inviteMember(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-001");

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should fail to invite non-existent user") {
    setupDb();
    gDb->setNextError("user not found");

    ProjectController projCtrl;
    Json::Value body;
    body["email"] = "nobody@example.com";
    auto req = makeAuthJsonReq(body);

    HttpResponsePtr resp;
    projCtrl.inviteMember(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-001");

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "User not found with that email");
}

// ---------------------------------------------------------------------------
// Step 9: Delete a task
// ---------------------------------------------------------------------------

TEST_CASE("should delete a task") {
    setupDb();
    gDb->setNextResult(Result{});

    TaskController taskCtrl;
    auto req = makeAuthReq();
    req->setParameter("id", "task-001");

    HttpResponsePtr resp;
    taskCtrl.deleteTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

// ---------------------------------------------------------------------------
// Step 10: Delete a column
// ---------------------------------------------------------------------------

TEST_CASE("should delete a column") {
    setupDb();
    gDb->setNextResult(Result{});

    ColumnController colCtrl;
    auto req = makeAuthReq();
    req->setParameter("id", "col-todo");

    HttpResponsePtr resp;
    colCtrl.deleteColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

// ---------------------------------------------------------------------------
// Step 11: Delete the project
// ---------------------------------------------------------------------------

TEST_CASE("should delete the project as owner") {
    setupDb();
    gDb->setNextResult(Result{});

    ProjectController projCtrl;
    auto req = makeAuthReq();

    HttpResponsePtr resp;
    projCtrl.deleteProject(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-001");

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should fail to delete project as non-owner") {
    setupDb();
    gDb->setNextError("not authorized: only owner can delete project");

    ProjectController projCtrl;
    auto req = makeAuthReq("different-user");

    HttpResponsePtr resp;
    projCtrl.deleteProject(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-001");

    CHECK(resp->statusCode() == k403Forbidden);
}

} // TEST_SUITE
