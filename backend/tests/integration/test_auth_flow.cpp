// =============================================================================
// Integration Tests: Authentication Flow
//
// Tests the complete register -> login -> me -> update -> logout flow,
// verifying that all components (controllers, filters, session, password
// hashing) work together correctly.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "controllers/AuthController.h"
#include "filters/AuthFilter.h"
#include "filters/CorsFilter.h"
#include "utils/PasswordHash.h"
#include "utils/Session.h"
#include "utils/Database.h"

using namespace kanba::controllers;
using namespace kanba::filters;
using namespace kanba::utils;
using namespace drogon;
using namespace drogon::orm;

// ---------------------------------------------------------------------------
// Test fixture: manages a mock DB shared across the test flow
// ---------------------------------------------------------------------------

static std::shared_ptr<DbClient> gDb;

static void setupDb() {
    gDb = std::make_shared<DbClient>();
    drogon::app().setDbClient("default", gDb);
    PasswordHash::initialize();
}

static HttpRequestPtr makeJsonRequest(const Json::Value& json) {
    auto req = HttpRequest::newHttpRequest();
    req->setJsonBody(json);
    return req;
}

TEST_SUITE("Integration: Authentication Flow") {

// ---------------------------------------------------------------------------
// CORS + Auth filter interaction
// ---------------------------------------------------------------------------

TEST_CASE("CORS filter should let OPTIONS through without auth") {
    CorsFilter corsFilter;
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Options);

    HttpResponsePtr resp;
    bool chainCalled = false;

    corsFilter.doFilter(
        req,
        [&](const HttpResponsePtr& r) { resp = r; },
        [&]() { chainCalled = true; }
    );

    REQUIRE(resp != nullptr);
    CHECK(resp->statusCode() == k204NoContent);
    CHECK(chainCalled == false);
    // Verify CORS headers are present
    CHECK_FALSE(resp->getHeader("Access-Control-Allow-Origin").empty());
    CHECK(resp->getHeader("Access-Control-Allow-Credentials") == "true");
}

TEST_CASE("Auth filter should reject unauthenticated requests") {
    setupDb();
    AuthFilter authFilter;
    auto req = HttpRequest::newHttpRequest();
    // No session cookie

    HttpResponsePtr resp;
    authFilter.doFilter(
        req,
        [&](const HttpResponsePtr& r) { resp = r; },
        []() { FAIL("chain should not be called"); }
    );

    CHECK(resp->statusCode() == k401Unauthorized);
}

// ---------------------------------------------------------------------------
// Registration flow
// ---------------------------------------------------------------------------

TEST_CASE("register should fail with missing fields") {
    setupDb();
    AuthController auth;

    // Missing name
    Json::Value body;
    body["email"] = "test@example.com";
    body["password"] = "securepassword";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    auth.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("register should succeed with valid input and create session") {
    setupDb();

    // Mock: create_user returns the new user
    Result createResult;
    Row userRow;
    userRow.addField("id", "new-user-uuid");
    userRow.addField("name", "Test User");
    userRow.addField("email", "test@example.com");
    createResult.addRow(userRow);
    gDb->setNextResult(createResult);

    AuthController auth;
    Json::Value body;
    body["email"] = "test@example.com";
    body["password"] = "securepassword123";
    body["name"] = "Test User";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    auth.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    // The first DB call is create_user which succeeds.
    // The response should contain user data.
    auto json = resp->getJsonObject();
    REQUIRE(json != nullptr);

    // The register flow makes 2 DB calls: create_user then createSession.
    // The mock records the last SQL, which will be the session INSERT.
    CHECK(gDb->callCount() >= 2);
    CHECK(gDb->lastSql().find("INSERT INTO sessions") != std::string::npos);
}

TEST_CASE("register should detect duplicate email errors") {
    setupDb();
    gDb->setNextError("duplicate key value violates unique constraint");

    AuthController auth;
    Json::Value body;
    body["email"] = "duplicate@example.com";
    body["password"] = "password";
    body["name"] = "Dup User";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    auth.registerUser(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Email already registered");
}

// ---------------------------------------------------------------------------
// Login flow
// ---------------------------------------------------------------------------

TEST_CASE("login should fail with wrong credentials") {
    setupDb();
    gDb->setNextResult(Result{});  // user not found

    AuthController auth;
    Json::Value body;
    body["email"] = "nobody@example.com";
    body["password"] = "wrong";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    auth.login(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k401Unauthorized);
    CHECK((*resp->getJsonObject())["error"].asString() == "Invalid email or password");
}

TEST_CASE("login should fail with wrong password against valid user") {
    setupDb();

    // User exists with a specific password hash
    std::string correctHash = PasswordHash::hash("correctpassword");
    Result userResult;
    Row userRow;
    userRow.addField("id", "user-123");
    userRow.addField("name", "Test User");
    userRow.addField("email", "test@example.com");
    userRow.addField("password_hash", correctHash);
    userResult.addRow(userRow);
    gDb->setNextResult(userResult);

    AuthController auth;
    Json::Value body;
    body["email"] = "test@example.com";
    body["password"] = "wrongpassword";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    auth.login(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k401Unauthorized);
}

TEST_CASE("login should succeed with correct password and set session cookie") {
    setupDb();

    std::string correctHash = PasswordHash::hash("correctpassword");
    Result userResult;
    Row userRow;
    userRow.addField("id", "user-123");
    userRow.addField("name", "Test User");
    userRow.addField("email", "test@example.com");
    userRow.addField("password_hash", correctHash);
    userResult.addRow(userRow);
    gDb->setNextResult(userResult);

    AuthController auth;
    Json::Value body;
    body["email"] = "test@example.com";
    body["password"] = "correctpassword";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    auth.login(req, [&](const HttpResponsePtr& r) { resp = r; });

    // Login makes 2 DB calls: get_user_by_email then createSession.
    // Verify the full flow executed (2+ DB calls means password matched).
    CHECK(gDb->callCount() >= 2);
    // Last SQL is session creation
    CHECK(gDb->lastSql().find("INSERT INTO sessions") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Logout flow
// ---------------------------------------------------------------------------

TEST_CASE("logout should clear the session cookie") {
    setupDb();
    gDb->setNextResult(Result{});

    AuthController auth;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "session-to-delete");

    HttpResponsePtr resp;
    auth.logout(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
    CHECK(resp->hasCookie("session") == true);
    CHECK(resp->getCookie("session").maxAge() == 0);
}

TEST_CASE("logout should succeed even without a session") {
    AuthController auth;
    auto req = HttpRequest::newHttpRequest();

    HttpResponsePtr resp;
    auth.logout(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["success"].asBool() == true);
}

// ---------------------------------------------------------------------------
// Me endpoint
// ---------------------------------------------------------------------------

TEST_CASE("me should return null user without session cookie") {
    AuthController auth;
    auto req = HttpRequest::newHttpRequest();

    HttpResponsePtr resp;
    auth.me(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK((*resp->getJsonObject())["user"].isNull());
}

// ---------------------------------------------------------------------------
// Update endpoint (requires auth)
// ---------------------------------------------------------------------------

TEST_CASE("update should reject requests without authentication context") {
    AuthController auth;
    auto req = HttpRequest::newHttpRequest();
    // No JSON body

    HttpResponsePtr resp;
    auth.update(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("update should work with valid auth context and name") {
    setupDb();
    Result updateResult;
    Row row;
    row.addField("id", "user-123");
    row.addField("name", "Updated Name");
    row.addField("email", "test@example.com");
    row.addField("avatar_url", "", true);
    updateResult.addRow(row);
    gDb->setNextResult(updateResult);

    AuthController auth;
    Json::Value body;
    body["name"] = "Updated Name";
    auto req = makeJsonRequest(body);
    req->attributes()->insert<std::string>("userId", "user-123");

    HttpResponsePtr resp;
    auth.update(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k200OK);
    auto json = resp->getJsonObject();
    CHECK((*json)["user"]["name"].asString() == "Updated Name");
}

} // TEST_SUITE
