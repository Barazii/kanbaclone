// =============================================================================
// Unit Tests: TaskController
//
// Tests createTask, updateTask, deleteTask, moveTask endpoints.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "controllers/TaskController.h"
#include "utils/Database.h"
#include "filters/AuthFilter.h"

using kanba::controllers::TaskController;
using namespace drogon;
using namespace drogon::orm;

static std::shared_ptr<DbClient> setupMockDb() {
    auto db = std::make_shared<DbClient>();
    drogon::app().setDbClient("default", db);
    return db;
}

static HttpRequestPtr makeAuthenticatedJsonRequest(const Json::Value& json) {
    auto req = HttpRequest::newHttpRequest();
    req->setJsonBody(json);
    req->attributes()->insert<std::string>("userId", "test-user-id");
    return req;
}

static HttpRequestPtr makeAuthenticatedRequest() {
    auto req = HttpRequest::newHttpRequest();
    req->attributes()->insert<std::string>("userId", "test-user-id");
    return req;
}

TEST_SUITE("TaskController::createTask") {

TEST_CASE("should return 400 when no JSON body") {
    TaskController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.createTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Column ID and title are required");
}

TEST_CASE("should return 400 when column_id is missing") {
    TaskController ctrl;
    Json::Value body;
    body["title"] = "My Task";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 400 when title is missing") {
    TaskController ctrl;
    Json::Value body;
    body["column_id"] = "col-1";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call create_task with correct SQL") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "task-1");
    row.addField("column_id", "col-1");
    row.addField("title", "New Task");
    row.addField("description", "", true);
    row.addField("priority", "medium");
    row.addField("position", "0");
    row.addField("assignee_id", "", true);
    row.addField("due_date", "", true);
    row.addField("tags", "", true);
    row.addField("created_at", "2025-01-01T00:00:00Z");
    r.addRow(row);
    db->setNextResult(r);

    TaskController ctrl;
    Json::Value body;
    body["column_id"] = "col-1";
    body["title"] = "New Task";
    auto req = makeAuthenticatedJsonRequest(body);

    ctrl.createTask(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("create_task") != std::string::npos);
}

TEST_CASE("should return 201 with task data on success") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "t-new");
    row.addField("column_id", "col-1");
    row.addField("title", "Important Task");
    row.addField("description", "Do this thing");
    row.addField("priority", "high");
    row.addField("position", "3");
    row.addField("assignee_id", "", true);
    row.addField("due_date", "", true);
    row.addField("tags", "", true);
    row.addField("created_at", "2025-01-15T10:00:00Z");
    r.addRow(row);
    db->setNextResult(r);

    TaskController ctrl;
    Json::Value body;
    body["column_id"] = "col-1";
    body["title"] = "Important Task";
    body["description"] = "Do this thing";
    body["priority"] = "high";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k201Created);
    auto json = resp->getJsonObject();
    CHECK((*json)["id"].asString() == "t-new");
    CHECK((*json)["title"].asString() == "Important Task");
    CHECK((*json)["priority"].asString() == "high");
    CHECK((*json)["position"].asInt() == 3);
}

TEST_CASE("should handle optional fields gracefully") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "t-1");
    row.addField("column_id", "col-1");
    row.addField("title", "Task");
    row.addField("description", "", true);
    row.addField("priority", "medium");
    row.addField("position", "0");
    row.addField("assignee_id", "user-1");
    row.addField("due_date", "2025-06-01T00:00:00Z");
    row.addField("tags", "[\"bug\",\"urgent\"]");
    row.addField("created_at", "2025-01-01T00:00:00Z");
    r.addRow(row);
    db->setNextResult(r);

    TaskController ctrl;
    Json::Value body;
    body["column_id"] = "col-1";
    body["title"] = "Task";
    body["assignee_id"] = "user-1";
    body["due_date"] = "2025-06-01T00:00:00Z";
    Json::Value tags(Json::arrayValue);
    tags.append("bug");
    tags.append("urgent");
    body["tags"] = tags;
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k201Created);
    auto json = resp->getJsonObject();
    CHECK((*json)["assignee_id"].asString() == "user-1");
    CHECK((*json)["due_date"].asString() == "2025-06-01T00:00:00Z");
}

TEST_CASE("should return 500 when DB returns empty result") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    TaskController ctrl;
    Json::Value body;
    body["column_id"] = "col-1";
    body["title"] = "Task";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("invalid column_id");

    TaskController ctrl;
    Json::Value body;
    body["column_id"] = "nonexistent";
    body["title"] = "Task";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE createTask

TEST_SUITE("TaskController::updateTask") {

TEST_CASE("should return 400 when no JSON body") {
    TaskController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.updateTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Task ID is required");
}

TEST_CASE("should return 400 when id is missing from body") {
    TaskController ctrl;
    Json::Value body;
    body["title"] = "Updated Title";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.updateTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call update_task DB function") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "t-1");
    row.addField("column_id", "col-1");
    row.addField("title", "Updated");
    row.addField("description", "", true);
    row.addField("priority", "high");
    row.addField("position", "0");
    row.addField("assignee_id", "", true);
    row.addField("due_date", "", true);
    r.addRow(row);
    db->setNextResult(r);

    TaskController ctrl;
    Json::Value body;
    body["id"] = "t-1";
    body["title"] = "Updated";
    body["priority"] = "high";
    auto req = makeAuthenticatedJsonRequest(body);

    ctrl.updateTask(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("update_task") != std::string::npos);
}

TEST_CASE("should return updated task data") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "t-1");
    row.addField("column_id", "col-1");
    row.addField("title", "New Title");
    row.addField("description", "New Desc");
    row.addField("priority", "low");
    row.addField("position", "2");
    row.addField("assignee_id", "", true);
    row.addField("due_date", "", true);
    r.addRow(row);
    db->setNextResult(r);

    TaskController ctrl;
    Json::Value body;
    body["id"] = "t-1";
    body["title"] = "New Title";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.updateTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k200OK);
    auto json = resp->getJsonObject();
    CHECK((*json)["title"].asString() == "New Title");
}

TEST_CASE("should return 404 when task not found") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    TaskController ctrl;
    Json::Value body;
    body["id"] = "nonexistent";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.updateTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k404NotFound);
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("constraint error");

    TaskController ctrl;
    Json::Value body;
    body["id"] = "t-1";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.updateTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE updateTask

TEST_SUITE("TaskController::deleteTask") {

TEST_CASE("should return 400 when id query parameter is missing") {
    TaskController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.deleteTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Task ID is required");
}

TEST_CASE("should call delete_task with task id and user id") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    TaskController ctrl;
    auto req = makeAuthenticatedRequest();
    req->setParameter("id", "task-to-delete");

    ctrl.deleteTask(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("delete_task") != std::string::npos);
}

TEST_CASE("should return success true") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    TaskController ctrl;
    auto req = makeAuthenticatedRequest();
    req->setParameter("id", "t-1");

    HttpResponsePtr resp;
    ctrl.deleteTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("permission denied");

    TaskController ctrl;
    auto req = makeAuthenticatedRequest();
    req->setParameter("id", "t-1");

    HttpResponsePtr resp;
    ctrl.deleteTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE deleteTask

TEST_SUITE("TaskController::moveTask") {

TEST_CASE("should return 400 when no JSON body") {
    TaskController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.moveTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Task ID and column ID are required");
}

TEST_CASE("should return 400 when task_id is missing") {
    TaskController ctrl;
    Json::Value body;
    body["column_id"] = "col-2";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.moveTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 400 when column_id is missing") {
    TaskController ctrl;
    Json::Value body;
    body["task_id"] = "t-1";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.moveTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call move_task DB function") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    TaskController ctrl;
    Json::Value body;
    body["task_id"] = "t-1";
    body["column_id"] = "col-2";
    body["position"] = 5;
    auto req = makeAuthenticatedJsonRequest(body);

    ctrl.moveTask(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("move_task") != std::string::npos);
}

TEST_CASE("should return success true on successful move") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    TaskController ctrl;
    Json::Value body;
    body["task_id"] = "t-1";
    body["column_id"] = "col-2";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.moveTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should default position to 0 when not specified") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    TaskController ctrl;
    Json::Value body;
    body["task_id"] = "t-1";
    body["column_id"] = "col-2";
    // No position field
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.moveTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    // Should not crash - position defaults to 0
    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("invalid column");

    TaskController ctrl;
    Json::Value body;
    body["task_id"] = "t-1";
    body["column_id"] = "bad-col";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.moveTask(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE moveTask
