// =============================================================================
// Unit Tests: Database utility
//
// Tests Database::callFunction SQL generation and error handling.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "utils/Database.h"

using kanba::utils::Database;
using namespace drogon;
using namespace drogon::orm;

static std::shared_ptr<DbClient> setupMockDb() {
    auto db = std::make_shared<DbClient>();
    drogon::app().setDbClient("default", db);
    return db;
}

TEST_SUITE("Database") {

// ---------------------------------------------------------------------------
// callFunction
// ---------------------------------------------------------------------------

TEST_CASE("callFunction should build correct SQL for zero-parameter function") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    Json::Value params(Json::objectValue);
    bool called = false;
    Database::callFunction("my_func", params,
        [&](const Result&) { called = true; },
        [](const DrogonDbException&) { FAIL("should not error"); }
    );
    CHECK(called == true);
    CHECK(db->lastSql() == "SELECT * FROM my_func()");
}

TEST_CASE("callFunction should build correct SQL for one-parameter function") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    Json::Value params;
    params["name"] = "test";

    Database::callFunction("get_user", params,
        [](const Result&) {},
        [](const DrogonDbException&) { FAIL("should not error"); }
    );
    CHECK(db->lastSql() == "SELECT * FROM get_user($1)");
}

TEST_CASE("callFunction should build correct SQL for multi-parameter function") {
    auto db = setupMockDb();
    db->setNextResult(Result{});

    Json::Value params;
    params["a"] = "1";
    params["b"] = "2";
    params["c"] = "3";

    Database::callFunction("do_something", params,
        [](const Result&) {},
        [](const DrogonDbException&) { FAIL("should not error"); }
    );
    CHECK(db->lastSql() == "SELECT * FROM do_something($1, $2, $3)");
}

TEST_CASE("callFunction should invoke error callback on DB error") {
    auto db = setupMockDb();
    db->setNextError("connection lost");

    bool errored = false;
    Database::callFunction("bad_func", Json::Value(Json::objectValue),
        [](const Result&) { FAIL("should not succeed"); },
        [&](const DrogonDbException& e) {
            errored = true;
            CHECK(std::string(e.what()) == "connection lost");
        }
    );
    CHECK(errored == true);
}

TEST_CASE("callFunction should not crash when DB client is null") {
    drogon::app().setDbClient("default", nullptr);

    // Should return early without calling either callback
    bool anyCalled = false;
    Database::callFunction("func", Json::Value(Json::objectValue),
        [&](const Result&) { anyCalled = true; },
        [&](const DrogonDbException&) { anyCalled = true; }
    );
    CHECK(anyCalled == false);
}

TEST_CASE("callFunction should pass results through to callback") {
    auto db = setupMockDb();
    Result r;
    Row row;
    row.addField("id", "42");
    row.addField("name", "test_name");
    r.addRow(row);
    db->setNextResult(r);

    bool called = false;
    Database::callFunction("get_item", Json::Value(Json::objectValue),
        [&](const Result& result) {
            called = true;
            CHECK_FALSE(result.empty());
            CHECK(result[0]["id"].as<std::string>() == "42");
            CHECK(result[0]["name"].as<std::string>() == "test_name");
        },
        [](const DrogonDbException&) { FAIL("should not error"); }
    );
    CHECK(called == true);
}

} // TEST_SUITE
