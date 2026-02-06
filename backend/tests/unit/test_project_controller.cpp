// =============================================================================
// Unit Tests: ProjectController
//
// Tests getProjects, createProject, getProject, deleteProject, inviteMember.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "controllers/ProjectController.h"
#include "utils/Database.h"
#include "filters/AuthFilter.h"

using kanba::controllers::ProjectController;
using kanba::filters::AuthFilter;
using namespace drogon;
using namespace drogon::orm;

static std::shared_ptr<DbClient> setupMockDb() {
    auto db = std::make_shared<DbClient>();
    drogon::app().setDbClient("default", db);
    return db;
}

static HttpRequestPtr makeAuthenticatedRequest() {
    auto req = HttpRequest::newHttpRequest();
    req->attributes()->insert<std::string>("userId", "test-user-id");
    return req;
}

static HttpRequestPtr makeAuthenticatedJsonRequest(const Json::Value& json) {
    auto req = HttpRequest::newHttpRequest();
    req->setJsonBody(json);
    req->attributes()->insert<std::string>("userId", "test-user-id");
    return req;
}

TEST_SUITE("ProjectController::getProjects") {

TEST_CASE("should call get_user_projects with user ID") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    ctrl.getProjects(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("get_user_projects") != std::string::npos);
}

TEST_CASE("should return empty array when user has no projects") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.getProjects(req, [&](const HttpResponsePtr& r) { resp = r; });

    auto json = resp->getJsonObject();
    REQUIRE(json != nullptr);
    CHECK(json->isArray());
    CHECK(json->size() == 0);
}

TEST_CASE("should return array of projects") {
    auto db = setupMockDb();
    Result r;
    Row row1;
    row1.addField("id", "p1");
    row1.addField("name", "Project 1");
    row1.addField("description", "", true);
    row1.addField("icon", "", true);
    row1.addField("owner_id", "test-user-id");
    row1.addField("task_count", "5");
    row1.addField("member_count", "3");
    row1.addField("created_at", "2025-01-01T00:00:00Z");
    r.addRow(row1);

    Row row2;
    row2.addField("id", "p2");
    row2.addField("name", "Project 2");
    row2.addField("description", "A description");
    row2.addField("icon", "rocket");
    row2.addField("owner_id", "test-user-id");
    row2.addField("task_count", "10");
    row2.addField("member_count", "1");
    row2.addField("created_at", "2025-02-01T00:00:00Z");
    r.addRow(row2);
    db->setNextResult(r);

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.getProjects(req, [&](const HttpResponsePtr& r) { resp = r; });

    auto json = resp->getJsonObject();
    REQUIRE(json->isArray());
    CHECK(json->size() == 2);
    CHECK((*json)[0]["id"].asString() == "p1");
    CHECK((*json)[0]["name"].asString() == "Project 1");
    CHECK((*json)[1]["id"].asString() == "p2");
    CHECK((*json)[1]["description"].asString() == "A description");
    CHECK((*json)[1]["icon"].asString() == "rocket");
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("connection refused");

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.getProjects(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE getProjects

TEST_SUITE("ProjectController::createProject") {

TEST_CASE("should return 400 when name is missing") {
    ProjectController ctrl;
    auto req = makeAuthenticatedJsonRequest(Json::Value(Json::objectValue));

    HttpResponsePtr resp;
    ctrl.createProject(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Project name is required");
}

TEST_CASE("should return 400 when no JSON body is provided") {
    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.createProject(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call create_project DB function") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "new-project-id");
    r.addRow(row);
    db->setNextResult(r);

    ProjectController ctrl;
    Json::Value body;
    body["name"] = "My Project";
    auto req = makeAuthenticatedJsonRequest(body);

    ctrl.createProject(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("create_project") != std::string::npos);
}

TEST_CASE("should return 201 with project id on success") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "new-id");
    r.addRow(row);
    db->setNextResult(r);

    ProjectController ctrl;
    Json::Value body;
    body["name"] = "My Project";
    body["description"] = "Desc";
    body["icon"] = "star";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createProject(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k201Created);
    CHECK((*resp->getJsonObject())["id"].asString() == "new-id");
    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should return 500 when create_project returns empty result") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ProjectController ctrl;
    Json::Value body;
    body["name"] = "Project";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createProject(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("constraint violation");

    ProjectController ctrl;
    Json::Value body;
    body["name"] = "Project";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.createProject(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE createProject

TEST_SUITE("ProjectController::deleteProject") {

TEST_CASE("should return success true on successful deletion") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.deleteProject(req, [&](const HttpResponsePtr& r) { resp = r; }, "project-id");

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should call delete_project with project id and user id") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    ctrl.deleteProject(req, [](const HttpResponsePtr&) {}, "proj-123");

    CHECK(db->lastSql().find("delete_project") != std::string::npos);
}

TEST_CASE("should return 403 when user is not the owner") {
    auto db = setupMockDb();
    db->setNextError("not authorized: only owner can delete");

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.deleteProject(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-id");

    CHECK(resp->statusCode() == k403Forbidden);
    CHECK((*resp->getJsonObject())["error"].asString() == "Only the project owner can delete this project");
}

TEST_CASE("should return 500 on generic database error") {
    auto db = setupMockDb();
    db->setNextError("something unexpected");

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.deleteProject(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-id");

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE deleteProject

TEST_SUITE("ProjectController::inviteMember") {

TEST_CASE("should return 400 when email is missing") {
    ProjectController ctrl;
    auto req = makeAuthenticatedJsonRequest(Json::Value(Json::objectValue));

    HttpResponsePtr resp;
    ctrl.inviteMember(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-id");

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Email is required");
}

TEST_CASE("should return 400 when no JSON body is provided") {
    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.inviteMember(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-id");

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call add_project_member DB function") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ProjectController ctrl;
    Json::Value body;
    body["email"] = "newmember@test.com";
    auto req = makeAuthenticatedJsonRequest(body);

    ctrl.inviteMember(req, [](const HttpResponsePtr&) {}, "proj-id");

    CHECK(db->lastSql().find("add_project_member") != std::string::npos);
}

TEST_CASE("should return success on valid invitation") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ProjectController ctrl;
    Json::Value body;
    body["email"] = "newmember@test.com";
    body["role"] = "admin";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.inviteMember(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-id");

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should return error when user not found") {
    auto db = setupMockDb();
    db->setNextError("user not found");

    ProjectController ctrl;
    Json::Value body;
    body["email"] = "nonexistent@test.com";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.inviteMember(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-id");

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "User not found with that email");
}

TEST_CASE("should return error when user already a member") {
    auto db = setupMockDb();
    db->setNextError("already a member");

    ProjectController ctrl;
    Json::Value body;
    body["email"] = "existing@test.com";
    auto req = makeAuthenticatedJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.inviteMember(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-id");

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "User is already a member of this project");
}

} // TEST_SUITE inviteMember

TEST_SUITE("ProjectController::getProject") {

TEST_CASE("should return 404 when project not found") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.getProject(req, [&](const HttpResponsePtr& r) { resp = r; }, "nonexistent-id");

    CHECK(resp->statusCode() == k404NotFound);
    CHECK((*resp->getJsonObject())["error"].asString() == "Project not found");
}

TEST_CASE("should return 500 on database error") {
    auto db = setupMockDb();
    db->setNextError("query failed");

    ProjectController ctrl;
    auto req = makeAuthenticatedRequest();

    HttpResponsePtr resp;
    ctrl.getProject(req, [&](const HttpResponsePtr& r) { resp = r; }, "proj-id");

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE getProject
