// =============================================================================
// Unit Tests: ColumnController
//
// Tests createColumn, updateColumn, deleteColumn endpoints.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "controllers/ColumnController.h"
#include "utils/Database.h"
#include "filters/AuthFilter.h"

using kanba::controllers::ColumnController;
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

TEST_SUITE("ColumnController::createColumn") {

TEST_CASE("should return 400 when no JSON body is provided") {
    ColumnController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Project ID and name are required");
}

TEST_CASE("should return 400 when project_id is missing") {
    ColumnController ctrl;
    Json::Value body;
    body["name"] = "To Do";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 400 when name is missing") {
    ColumnController ctrl;
    Json::Value body;
    body["project_id"] = "proj-1";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call create_column DB function") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "col-1");
    row.addField("project_id", "proj-1");
    row.addField("name", "To Do");
    row.addField("color", "", true);
    row.addField("position", "0");
    r.addRow(row);
    db->setNextResult(r);

    ColumnController ctrl;
    Json::Value body;
    body["project_id"] = "proj-1";
    body["name"] = "To Do";
    auto req = makeAuthenticatedJsonRequest(body);

    ctrl.createColumn(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("create_column") != std::string::npos);
}

TEST_CASE("should return 201 with column data on success") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "col-new");
    row.addField("project_id", "proj-1");
    row.addField("name", "In Progress");
    row.addField("color", "#ff0000");
    row.addField("position", "1");
    r.addRow(row);
    db->setNextResult(r);

    ColumnController ctrl;
    Json::Value body;
    body["project_id"] = "proj-1";
    body["name"] = "In Progress";
    body["color"] = "#ff0000";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k201Created);
    auto json = resp->getJsonObject();
    CHECK((*json)["id"].asString() == "col-new");
    CHECK((*json)["name"].asString() == "In Progress");
    CHECK((*json)["color"].asString() == "#ff0000");
    CHECK((*json)["position"].asInt() == 1);
}

TEST_CASE("should return 500 when DB returns empty result") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ColumnController ctrl;
    Json::Value body;
    body["project_id"] = "proj-1";
    body["name"] = "Column";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("foreign key constraint");

    ColumnController ctrl;
    Json::Value body;
    body["project_id"] = "invalid-proj";
    body["name"] = "Column";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE createColumn

TEST_SUITE("ColumnController::updateColumn") {

TEST_CASE("should return 400 when no JSON body") {
    ColumnController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.updateColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Column ID is required");
}

TEST_CASE("should return 400 when id is missing") {
    ColumnController ctrl;
    Json::Value body;
    body["name"] = "Updated Name";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.updateColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call update_column DB function") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "col-1");
    row.addField("name", "Updated");
    row.addField("color", "#00ff00");
    row.addField("position", "0");
    r.addRow(row);
    db->setNextResult(r);

    ColumnController ctrl;
    Json::Value body;
    body["id"] = "col-1";
    body["name"] = "Updated";
    body["color"] = "#00ff00";
    auto req = makeAuthenticatedJsonRequest(body);

    ctrl.updateColumn(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("update_column") != std::string::npos);
}

TEST_CASE("should return updated column data") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "col-1");
    row.addField("name", "Done");
    row.addField("color", "", true);
    row.addField("position", "2");
    r.addRow(row);
    db->setNextResult(r);

    ColumnController ctrl;
    Json::Value body;
    body["id"] = "col-1";
    body["name"] = "Done";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.updateColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k200OK);
    auto json = resp->getJsonObject();
    CHECK((*json)["name"].asString() == "Done");
    CHECK((*json)["position"].asInt() == 2);
}

TEST_CASE("should return 404 when column not found") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ColumnController ctrl;
    Json::Value body;
    body["id"] = "nonexistent";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.updateColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k404NotFound);
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("update failed");

    ColumnController ctrl;
    Json::Value body;
    body["id"] = "col-1";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.updateColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE updateColumn

TEST_SUITE("ColumnController::deleteColumn") {

TEST_CASE("should return 400 when id query parameter is missing") {
    ColumnController ctrl;
    auto req = makeAuthenticatedRequest();
    // No parameter set

    HttpResponsePtr resp;
    ctrl.deleteColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Column ID is required");
}

TEST_CASE("should call delete_column DB function") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ColumnController ctrl;
    auto req = makeAuthenticatedRequest();
    req->setParameter("id", "col-to-delete");

    ctrl.deleteColumn(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("delete_column") != std::string::npos);
}

TEST_CASE("should return success true on deletion") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ColumnController ctrl;
    auto req = makeAuthenticatedRequest();
    req->setParameter("id", "col-1");

    HttpResponsePtr resp;
    ctrl.deleteColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("referential integrity");

    ColumnController ctrl;
    auto req = makeAuthenticatedRequest();
    req->setParameter("id", "col-1");

    HttpResponsePtr resp;
    ctrl.deleteColumn(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE deleteColumn
