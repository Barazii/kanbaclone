// =============================================================================
// Unit Tests: Session
//
// Tests session creation, validation, deletion, and cleanup logic
// via a mock database.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "utils/Session.h"
#include "utils/Database.h"

using kanba::utils::Session;
using namespace drogon;
using namespace drogon::orm;

// Helper: set up a mock DB and register it with the mock app
static std::shared_ptr<DbClient> setupMockDb() {
    auto db = std::make_shared<DbClient>();
    drogon::app().setDbClient("default", db);
    return db;
}

TEST_SUITE("Session") {

// ---------------------------------------------------------------------------
// createSession
// ---------------------------------------------------------------------------

TEST_CASE("createSession should succeed when DB call succeeds") {
    auto db = setupMockDb();
    Result r;
    db->setNextResult(r);

    bool success = false;
    Session::createSession("session-123", "user-456", [&](bool s) { success = s; });
    CHECK(success == true);
}

TEST_CASE("createSession should call DB with correct INSERT SQL") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    Session::createSession("sid", "uid", [](bool) {});
    CHECK(db->lastSql().find("INSERT INTO sessions") != std::string::npos);
    CHECK(db->lastSql().find("ON CONFLICT") != std::string::npos);
}

TEST_CASE("createSession should fail when DB returns error") {
    auto db = setupMockDb();
    db->setNextError("connection refused");

    bool success = true;
    Session::createSession("sid", "uid", [&](bool s) { success = s; });
    CHECK(success == false);
}

TEST_CASE("createSession should fail when DB client is null") {
    drogon::app().setDbClient("default", nullptr);

    bool success = true;
    Session::createSession("sid", "uid", [&](bool s) { success = s; });
    CHECK(success == false);
}

// ---------------------------------------------------------------------------
// getUserIdFromSession
// ---------------------------------------------------------------------------

TEST_CASE("getUserIdFromSession should return userId when session exists") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("user_id", "user-789");
    r.addRow(row);
    db->setNextResult(r);

    std::optional<std::string> result;
    Session::getUserIdFromSession("valid-session", [&](std::optional<std::string> uid) {
        result = uid;
    });
    REQUIRE(result.has_value());
    CHECK(*result == "user-789");
}

TEST_CASE("getUserIdFromSession should return nullopt when session does not exist") {
    auto db = setupMockDb();
    db->setNextResult(Result{});  // empty result

    std::optional<std::string> result = std::string("should-be-cleared");
    Session::getUserIdFromSession("expired-session", [&](std::optional<std::string> uid) {
        result = uid;
    });
    CHECK_FALSE(result.has_value());
}

TEST_CASE("getUserIdFromSession should return nullopt on DB error") {
    auto db = setupMockDb();
    db->setNextError("timeout");

    std::optional<std::string> result = std::string("should-be-cleared");
    Session::getUserIdFromSession("sid", [&](std::optional<std::string> uid) {
        result = uid;
    });
    CHECK_FALSE(result.has_value());
}

TEST_CASE("getUserIdFromSession should return nullopt when DB client is null") {
    drogon::app().setDbClient("default", nullptr);

    std::optional<std::string> result = std::string("should-be-cleared");
    Session::getUserIdFromSession("sid", [&](std::optional<std::string> uid) {
        result = uid;
    });
    CHECK_FALSE(result.has_value());
}

TEST_CASE("getUserIdFromSession SQL should query with expiry check") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    Session::getUserIdFromSession("sid", [](std::optional<std::string>) {});
    CHECK(db->lastSql().find("expires_at > NOW()") != std::string::npos);
}

// ---------------------------------------------------------------------------
// deleteSession
// ---------------------------------------------------------------------------

TEST_CASE("deleteSession should succeed when DB call succeeds") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    bool success = false;
    Session::deleteSession("sid", [&](bool s) { success = s; });
    CHECK(success == true);
}

TEST_CASE("deleteSession should use DELETE SQL") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    Session::deleteSession("sid", [](bool) {});
    CHECK(db->lastSql().find("DELETE FROM sessions") != std::string::npos);
}

TEST_CASE("deleteSession should fail on DB error") {
    auto db = setupMockDb();
    db->setNextError("disk full");

    bool success = true;
    Session::deleteSession("sid", [&](bool s) { success = s; });
    CHECK(success == false);
}

TEST_CASE("deleteSession should fail when DB client is null") {
    drogon::app().setDbClient("default", nullptr);

    bool success = true;
    Session::deleteSession("sid", [&](bool s) { success = s; });
    CHECK(success == false);
}

// ---------------------------------------------------------------------------
// isValidSession
// ---------------------------------------------------------------------------

TEST_CASE("isValidSession should return true when session has a user") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("user_id", "user-abc");
    r.addRow(row);
    db->setNextResult(r);

    bool valid = false;
    Session::isValidSession("sid", [&](bool v) { valid = v; });
    CHECK(valid == true);
}

TEST_CASE("isValidSession should return false when session not found") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    bool valid = true;
    Session::isValidSession("sid", [&](bool v) { valid = v; });
    CHECK(valid == false);
}

// ---------------------------------------------------------------------------
// cleanupExpiredSessions
// ---------------------------------------------------------------------------

TEST_CASE("cleanupExpiredSessions should report the number of deleted rows") {
    auto db = setupMockDb();
    Result r;
    r.setAffectedRows(5);
    db->setNextResult(r);

    int count = -1;
    Session::cleanupExpiredSessions([&](int c) { count = c; });
    CHECK(count == 5);
}

TEST_CASE("cleanupExpiredSessions should return 0 on DB error") {
    auto db = setupMockDb();
    db->setNextError("permission denied");

    int count = -1;
    Session::cleanupExpiredSessions([&](int c) { count = c; });
    CHECK(count == 0);
}

TEST_CASE("cleanupExpiredSessions should return 0 when DB client is null") {
    drogon::app().setDbClient("default", nullptr);

    int count = -1;
    Session::cleanupExpiredSessions([&](int c) { count = c; });
    CHECK(count == 0);
}

TEST_CASE("cleanupExpiredSessions SQL should filter by expires_at") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    Session::cleanupExpiredSessions([](int) {});
    CHECK(db->lastSql().find("expires_at < NOW()") != std::string::npos);
}

} // TEST_SUITE
