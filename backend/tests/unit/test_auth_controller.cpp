// =============================================================================
// Unit Tests: AuthController
//
// Tests login, register, logout, me, and update endpoints.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "controllers/AuthController.h"
#include "utils/Database.h"
#include "utils/Session.h"
#include "utils/PasswordHash.h"
#include "filters/AuthFilter.h"

using kanba::controllers::AuthController;
using kanba::filters::AuthFilter;
using namespace drogon;
using namespace drogon::orm;

static std::shared_ptr<DbClient> setupMockDb() {
    auto db = std::make_shared<DbClient>();
    drogon::app().setDbClient("default", db);
    return db;
}

// Helper: create a request with JSON body
static HttpRequestPtr makeJsonRequest(const Json::Value& json) {
    auto req = HttpRequest::newHttpRequest();
    req->setJsonBody(json);
    return req;
}

TEST_SUITE("AuthController::login") {

TEST_CASE("should return 400 when no JSON body is provided") {
    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();
    // No JSON body set

    HttpResponsePtr resp;
    ctrl.login(req, [&](const HttpResponsePtr& r) { resp = r; });

    REQUIRE(resp != nullptr);
    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Email and password are required");
}

TEST_CASE("should return 400 when email is missing") {
    AuthController ctrl;
    Json::Value body;
    body["password"] = "secret";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.login(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 400 when password is missing") {
    AuthController ctrl;
    Json::Value body;
    body["email"] = "user@test.com";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.login(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 401 when user is not found in database") {
    auto db = setupMockDb();
    db->setNextResult(Result{});  // empty = user not found

    AuthController ctrl;
    Json::Value body;
    body["email"] = "nobody@test.com";
    body["password"] = "secret";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.login(req, [&](const HttpResponsePtr& r) { resp = r; });

    REQUIRE(resp != nullptr);
    CHECK(resp->statusCode() == k401Unauthorized);
    CHECK((*resp->getJsonObject())["error"].asString() == "Invalid email or password");
}

TEST_CASE("should return 500 on database error during login") {
    auto db = setupMockDb();
    db->setNextError("connection timeout");

    AuthController ctrl;
    Json::Value body;
    body["email"] = "user@test.com";
    body["password"] = "secret";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.login(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
    CHECK((*resp->getJsonObject())["error"].asString() == "Database error");
}

TEST_CASE("should query get_user_by_email function") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    AuthController ctrl;
    Json::Value body;
    body["email"] = "test@test.com";
    body["password"] = "pw";
    auto req = makeJsonRequest(body);

    ctrl.login(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("get_user_by_email") != std::string::npos);
}

} // TEST_SUITE login

TEST_SUITE("AuthController::registerUser") {

TEST_CASE("should return 400 when JSON body is missing") {
    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();

    HttpResponsePtr resp;
    ctrl.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Email, password, and name are required");
}

TEST_CASE("should return 400 when name is missing") {
    AuthController ctrl;
    Json::Value body;
    body["email"] = "user@test.com";
    body["password"] = "secret";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 400 when email is missing") {
    AuthController ctrl;
    Json::Value body;
    body["password"] = "secret";
    body["name"] = "Test User";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 400 when password is missing") {
    AuthController ctrl;
    Json::Value body;
    body["email"] = "user@test.com";
    body["name"] = "Test User";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call create_user DB function with valid input") {
    kanba::utils::PasswordHash::initialize();
    auto db = setupMockDb();

    Result r;
    Row row;
    row.addField("id", "new-user-id");
    row.addField("name", "Test User");
    row.addField("email", "user@test.com");
    r.addRow(row);
    db->setNextResult(r);

    AuthController ctrl;
    Json::Value body;
    body["email"] = "user@test.com";
    body["password"] = "secret123";
    body["name"] = "Test User";
    auto req = makeJsonRequest(body);

    ctrl.registerUser(req, [](const HttpResponsePtr&) {});

    // The register flow makes 2 DB calls: create_user then createSession.
    // The mock DB records the last SQL, which will be the session INSERT.
    // We verify that at least 2 calls were made (create_user + session).
    CHECK(db->callCount() >= 2);
    // The last SQL is the session creation (INSERT INTO sessions)
    CHECK(db->lastSql().find("INSERT INTO sessions") != std::string::npos);
}

TEST_CASE("should handle duplicate email error") {
    kanba::utils::PasswordHash::initialize();
    auto db = setupMockDb();
    db->setNextError("duplicate key value violates unique constraint");

    AuthController ctrl;
    Json::Value body;
    body["email"] = "existing@test.com";
    body["password"] = "secret";
    body["name"] = "Dup User";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Email already registered");
}

TEST_CASE("should return generic error for non-duplicate DB errors") {
    kanba::utils::PasswordHash::initialize();
    auto db = setupMockDb();
    db->setNextError("some other database error");

    AuthController ctrl;
    Json::Value body;
    body["email"] = "user@test.com";
    body["password"] = "secret";
    body["name"] = "User";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Database error");
}

} // TEST_SUITE registerUser

TEST_SUITE("AuthController::logout") {

TEST_CASE("should return success true") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "some-session-id");

    HttpResponsePtr resp;
    ctrl.logout(req, [&](const HttpResponsePtr& r) { resp = r; });

    REQUIRE(resp != nullptr);
    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should clear session cookie") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "sid");

    HttpResponsePtr resp;
    ctrl.logout(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->hasCookie("session") == true);
    auto cookie = resp->getCookie("session");
    CHECK(cookie.maxAge() == 0);
}

TEST_CASE("should succeed even without a session cookie") {
    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();

    HttpResponsePtr resp;
    ctrl.logout(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

TEST_CASE("should attempt to delete session from database when cookie exists") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "session-to-delete");

    ctrl.logout(req, [](const HttpResponsePtr&) {});

    CHECK(db->callCount() >= 1);
    CHECK(db->lastSql().find("DELETE FROM sessions") != std::string::npos);
}

} // TEST_SUITE logout

TEST_SUITE("AuthController::me") {

TEST_CASE("should return null user when no session cookie") {
    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();

    HttpResponsePtr resp;
    ctrl.me(req, [&](const HttpResponsePtr& r) { resp = r; });

    REQUIRE(resp != nullptr);
    auto json = resp->getJsonObject();
    CHECK(json->isMember("user"));
    CHECK((*json)["user"].isNull());
}

TEST_CASE("should return null user when session is invalid") {
    auto db = setupMockDb();
    db->setNextResult(Result{});  // session not found

    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "bad-session");

    HttpResponsePtr resp;
    ctrl.me(req, [&](const HttpResponsePtr& r) { resp = r; });

    auto json = resp->getJsonObject();
    CHECK((*json)["user"].isNull());
}

} // TEST_SUITE me

TEST_SUITE("AuthController::update") {

TEST_CASE("should return 400 when name is missing from body") {
    AuthController ctrl;
    auto req = HttpRequest::newHttpRequest();
    // No JSON body

    HttpResponsePtr resp;
    ctrl.update(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Name is required");
}

TEST_CASE("should return 400 when body has no name field") {
    AuthController ctrl;
    Json::Value body;
    body["something_else"] = "value";
    auto req = makeJsonRequest(body);
    req->attributes()->insert<std::string>("userId", "user-id");

    HttpResponsePtr resp;
    ctrl.update(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should call UPDATE SQL with correct parameters") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "user-id");
    row.addField("name", "New Name");
    row.addField("email", "user@test.com");
    row.addField("avatar_url", "", true);
    r.addRow(row);
    db->setNextResult(r);

    AuthController ctrl;
    Json::Value body;
    body["name"] = "New Name";
    auto req = makeJsonRequest(body);
    req->attributes()->insert<std::string>("userId", "user-id");

    ctrl.update(req, [](const HttpResponsePtr&) {});

    CHECK(db->lastSql().find("UPDATE users SET name") != std::string::npos);
}

TEST_CASE("should return updated user data on success") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "u1");
    row.addField("name", "Updated Name");
    row.addField("email", "u@test.com");
    row.addField("avatar_url", "", true);
    r.addRow(row);
    db->setNextResult(r);

    AuthController ctrl;
    Json::Value body;
    body["name"] = "Updated Name";
    auto req = makeJsonRequest(body);
    req->attributes()->insert<std::string>("userId", "u1");

    HttpResponsePtr resp;
    ctrl.update(req, [&](const HttpResponsePtr& r) { resp = r; });

    auto json = resp->getJsonObject();
    CHECK((*json)["user"]["id"].asString() == "u1");
    CHECK((*json)["user"]["name"].asString() == "Updated Name");
    CHECK((*json)["user"]["email"].asString() == "u@test.com");
}

TEST_CASE("should return 404 when user not found") {
    auto db = setupMockDb();
    db->setNextResult(Result{});  // empty = user not found

    AuthController ctrl;
    Json::Value body;
    body["name"] = "Name";
    auto req = makeJsonRequest(body);
    req->attributes()->insert<std::string>("userId", "nonexistent");

    HttpResponsePtr resp;
    ctrl.update(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k404NotFound);
}

TEST_CASE("should return 500 on database error during update") {
    auto db = setupMockDb();
    db->setNextError("disk full");

    AuthController ctrl;
    Json::Value body;
    body["name"] = "Name";
    auto req = makeJsonRequest(body);
    req->attributes()->insert<std::string>("userId", "uid");

    HttpResponsePtr resp;
    ctrl.update(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k500InternalServerError);
}

} // TEST_SUITE update
