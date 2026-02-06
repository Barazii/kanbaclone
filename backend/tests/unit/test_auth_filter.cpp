// =============================================================================
// Unit Tests: AuthFilter
//
// Tests session cookie extraction, session validation, and user ID attribute
// injection into the request.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "filters/AuthFilter.h"
#include "utils/Session.h"
#include "utils/Database.h"

using kanba::filters::AuthFilter;
using namespace drogon;
using namespace drogon::orm;

static std::shared_ptr<DbClient> setupMockDb() {
    auto db = std::make_shared<DbClient>();
    drogon::app().setDbClient("default", db);
    return db;
}

TEST_SUITE("AuthFilter") {

// ---------------------------------------------------------------------------
// Missing session cookie
// ---------------------------------------------------------------------------

TEST_CASE("should return 401 when no session cookie is present") {
    AuthFilter filter;
    auto req = HttpRequest::newHttpRequest();

    HttpResponsePtr capturedResp;
    bool chainCalled = false;

    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        [&]() { chainCalled = true; }
    );

    REQUIRE(capturedResp != nullptr);
    CHECK(capturedResp->statusCode() == k401Unauthorized);
    CHECK(chainCalled == false);

    auto json = capturedResp->getJsonObject();
    REQUIRE(json != nullptr);
    CHECK((*json)["error"].asString() == "Unauthorized");
}

TEST_CASE("should return 401 when session cookie is empty string") {
    AuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "");

    HttpResponsePtr capturedResp;
    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        []() {}
    );

    CHECK(capturedResp->statusCode() == k401Unauthorized);
}

// ---------------------------------------------------------------------------
// Invalid session (not found in DB)
// ---------------------------------------------------------------------------

TEST_CASE("should return 401 when session is not found in database") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    AuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "invalid-session-id");

    HttpResponsePtr capturedResp;
    bool chainCalled = false;

    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        [&]() { chainCalled = true; }
    );

    CHECK(capturedResp != nullptr);
    CHECK(capturedResp->statusCode() == k401Unauthorized);
    CHECK(chainCalled == false);
}

TEST_CASE("should return 401 when DB error occurs during session lookup") {
    auto db = setupMockDb();
    db->setNextError("database connection failed");

    AuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "some-session-id");

    HttpResponsePtr capturedResp;
    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        []() {}
    );

    CHECK(capturedResp != nullptr);
    CHECK(capturedResp->statusCode() == k401Unauthorized);
}

// ---------------------------------------------------------------------------
// Valid session
// ---------------------------------------------------------------------------

TEST_CASE("should call chain callback when session is valid") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("user_id", "user-abc-123");
    r.addRow(row);
    db->setNextResult(r);

    AuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "valid-session");

    bool chainCalled = false;
    filter.doFilter(
        req,
        [](const HttpResponsePtr&) {},
        [&]() { chainCalled = true; }
    );

    CHECK(chainCalled == true);
}

TEST_CASE("should inject userId into request attributes when session is valid") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("user_id", "user-xyz-789");
    r.addRow(row);
    db->setNextResult(r);

    AuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "valid-session");

    filter.doFilter(
        req,
        [](const HttpResponsePtr&) {},
        []() {}
    );

    std::string userId = req->attributes()->get<std::string>("userId");
    CHECK(userId == "user-xyz-789");
}

TEST_CASE("should query database for session validation") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    AuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setCookie("session", "test-session-id");

    filter.doFilter(
        req,
        [](const HttpResponsePtr&) {},
        []() {}
    );

    CHECK(db->callCount() > 0);
    CHECK(db->lastSql().find("sessions") != std::string::npos);
}

} // TEST_SUITE
