#include "doctest.h"
#include "http_test_client.h"
#include "test_helpers.h"

TEST_SUITE("Tasks") {

    TEST_CASE("POST /api/tasks - requires auth") {
        httptest::HttpTestClient client;
        Json::Value body;
        body["column_id"] = "x";
        body["title"] = "y";
        auto resp = client.post("/api/tasks", body);
        CHECK(resp.statusCode == 401);
    }

    TEST_CASE("POST /api/tasks - creates task in column") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_create");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Task Project");
        auto columnId = getFirstColumnId(client, projectId);

        Json::Value body;
        body["column_id"] = columnId;
        body["title"] = "My Task";
        body["priority"] = "high";
        auto resp = client.post("/api/tasks", body);

        CHECK(resp.statusCode == 201);
        CHECK(!resp.body["id"].asString().empty());
        CHECK(resp.body["title"].asString() == "My Task");
        CHECK(resp.body["priority"].asString() == "high");
        CHECK(resp.body["column_id"].asString() == columnId);
        CHECK(resp.body["position"].asInt() == 0);
        CHECK(resp.body.isMember("created_at"));
    }

    TEST_CASE("POST /api/tasks - with optional fields") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_opts");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Task Project");
        auto columnId = getFirstColumnId(client, projectId);

        Json::Value tags(Json::arrayValue);
        tags.append("bug");
        tags.append("urgent");

        Json::Value body;
        body["column_id"] = columnId;
        body["title"] = "Detailed Task";
        body["description"] = "A detailed description";
        body["priority"] = "low";
        body["tags"] = tags;
        auto resp = client.post("/api/tasks", body);

        CHECK(resp.statusCode == 201);
        CHECK(resp.body["description"].asString() == "A detailed description");
        CHECK(resp.body["priority"].asString() == "low");
        CHECK(resp.body["tags"].isArray());
        CHECK(resp.body["tags"].size() == 2);
    }

    TEST_CASE("POST /api/tasks - missing column_id returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_nocol");
        auto client = registerAndLogin(email, "Pass123", "User");

        Json::Value body;
        body["title"] = "X";
        auto resp = client.post("/api/tasks", body);
        CHECK(resp.statusCode == 400);
    }

    TEST_CASE("POST /api/tasks - missing title returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_notitle");
        auto client = registerAndLogin(email, "Pass123", "User");

        Json::Value body;
        body["column_id"] = "some-id";
        auto resp = client.post("/api/tasks", body);
        CHECK(resp.statusCode == 400);
    }

    TEST_CASE("PUT /api/tasks - updates task fields") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_update");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Task Project");
        auto columnId = getFirstColumnId(client, projectId);
        auto taskId = createTask(client, columnId, "Original Title");

        Json::Value body;
        body["id"] = taskId;
        body["title"] = "Updated Title";
        body["priority"] = "low";
        auto resp = client.put("/api/tasks", body);

        CHECK(resp.statusCode == 200);
        CHECK(resp.body["title"].asString() == "Updated Title");
        CHECK(resp.body["priority"].asString() == "low");
    }

    TEST_CASE("PUT /api/tasks - missing id returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_noid");
        auto client = registerAndLogin(email, "Pass123", "User");

        Json::Value body;
        body["title"] = "X";
        auto resp = client.put("/api/tasks", body);
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Task ID is required");
    }

    TEST_CASE("DELETE /api/tasks?id=xxx - deletes task") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_del");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Task Project");
        auto columnId = getFirstColumnId(client, projectId);
        auto taskId = createTask(client, columnId, "Deletable");

        auto resp = client.del("/api/tasks?id=" + taskId);
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["success"].asBool() == true);
    }

    TEST_CASE("DELETE /api/tasks - missing id returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_delmissing");
        auto client = registerAndLogin(email, "Pass123", "User");

        auto resp = client.del("/api/tasks");
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Task ID is required");
    }

    TEST_CASE("POST /api/tasks/move - moves task to different column") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_move");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Move Project");

        // Get the two default columns
        auto columns = getProjectColumns(client, projectId);
        REQUIRE(columns.size() == 2);
        auto col1Id = columns[0].first;  // "To Do"
        auto col2Id = columns[1].first;  // "Done"

        // Create task in first column
        auto taskId = createTask(client, col1Id, "Movable Task");

        // Move to second column
        Json::Value body;
        body["task_id"] = taskId;
        body["column_id"] = col2Id;
        body["position"] = 0;
        auto resp = client.post("/api/tasks/move", body);

        CHECK(resp.statusCode == 200);
        CHECK(resp.body["success"].asBool() == true);

        // Verify task is now in second column
        auto projResp = client.get("/api/projects/" + projectId);
        bool foundInCol2 = false;
        for (const auto& col : projResp.body["columns"]) {
            if (col["id"].asString() == col2Id) {
                for (const auto& task : col["tasks"]) {
                    if (task["id"].asString() == taskId) {
                        foundInCol2 = true;
                    }
                }
            }
        }
        CHECK(foundInCol2);
    }

    TEST_CASE("POST /api/tasks/move - missing fields returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("task_movebad");
        auto client = registerAndLogin(email, "Pass123", "User");

        SUBCASE("missing column_id") {
            Json::Value body;
            body["task_id"] = "x";
            auto resp = client.post("/api/tasks/move", body);
            CHECK(resp.statusCode == 400);
        }

        SUBCASE("missing task_id") {
            Json::Value body;
            body["column_id"] = "x";
            auto resp = client.post("/api/tasks/move", body);
            CHECK(resp.statusCode == 400);
        }
    }

}
