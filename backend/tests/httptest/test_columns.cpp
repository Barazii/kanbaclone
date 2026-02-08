#include "doctest.h"
#include "http_test_client.h"
#include "test_helpers.h"

TEST_SUITE("Columns") {

    TEST_CASE("POST /api/columns - requires auth") {
        httptest::HttpTestClient client;
        Json::Value body;
        body["project_id"] = "x";
        body["name"] = "y";
        auto resp = client.post("/api/columns", body);
        CHECK(resp.statusCode == 401);
    }

    TEST_CASE("POST /api/columns - creates column") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("col_create");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Col Project");

        Json::Value body;
        body["project_id"] = projectId;
        body["name"] = "In Progress";
        body["color"] = "#f59e0b";
        auto resp = client.post("/api/columns", body);

        CHECK(resp.statusCode == 201);
        CHECK(!resp.body["id"].asString().empty());
        CHECK(resp.body["name"].asString() == "In Progress");
        CHECK(resp.body["color"].asString() == "#f59e0b");
        CHECK(resp.body["position"].isInt());
        // Default columns are "To Do" (0) and "Done" (1), so new column is position 2
        CHECK(resp.body["position"].asInt() == 2);
    }

    TEST_CASE("POST /api/columns - missing project_id returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("col_nopid");
        auto client = registerAndLogin(email, "Pass123", "User");

        Json::Value body;
        body["name"] = "X";
        auto resp = client.post("/api/columns", body);
        CHECK(resp.statusCode == 400);
    }

    TEST_CASE("POST /api/columns - missing name returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("col_noname");
        auto client = registerAndLogin(email, "Pass123", "User");

        Json::Value body;
        body["project_id"] = "some-id";
        auto resp = client.post("/api/columns", body);
        CHECK(resp.statusCode == 400);
    }

    TEST_CASE("PUT /api/columns - updates column name and color") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("col_update");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Col Project");

        // Get default column ID
        auto columns = getProjectColumns(client, projectId);
        auto columnId = columns[0].first;

        Json::Value body;
        body["id"] = columnId;
        body["name"] = "Renamed";
        body["color"] = "#ff0000";
        auto resp = client.put("/api/columns", body);

        CHECK(resp.statusCode == 200);
        CHECK(resp.body["name"].asString() == "Renamed");
        CHECK(resp.body["color"].asString() == "#ff0000");
    }

    TEST_CASE("PUT /api/columns - missing id returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("col_noid");
        auto client = registerAndLogin(email, "Pass123", "User");

        Json::Value body;
        body["name"] = "X";
        auto resp = client.put("/api/columns", body);
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Column ID is required");
    }

    TEST_CASE("DELETE /api/columns?id=xxx - deletes column") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("col_del");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Col Project");

        // Create an extra column to delete
        Json::Value body;
        body["project_id"] = projectId;
        body["name"] = "Temp Column";
        auto createResp = client.post("/api/columns", body);
        auto extraColumnId = createResp.body["id"].asString();

        auto resp = client.del("/api/columns?id=" + extraColumnId);
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["success"].asBool() == true);

        // Verify column is gone
        auto columns = getProjectColumns(client, projectId);
        CHECK(columns.size() == 2);  // Only the 2 default columns remain
    }

    TEST_CASE("DELETE /api/columns - missing id returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("col_delmissing");
        auto client = registerAndLogin(email, "Pass123", "User");

        auto resp = client.del("/api/columns");
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Column ID is required");
    }

}
