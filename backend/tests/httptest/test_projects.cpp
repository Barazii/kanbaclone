#include "doctest.h"
#include "http_test_client.h"
#include "test_helpers.h"

TEST_SUITE("Projects") {

    TEST_CASE("GET /api/projects - requires auth") {
        httptest::HttpTestClient client;
        auto resp = client.get("/api/projects");
        CHECK(resp.statusCode == 401);
    }

    TEST_CASE("GET /api/projects - returns empty list initially") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("proj_empty");
        auto client = registerAndLogin(email, "Pass123", "Empty User");

        auto resp = client.get("/api/projects");
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["projects"].isArray());
        CHECK(resp.body["projects"].size() == 0);
    }

    TEST_CASE("POST /api/projects - creates project") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("proj_create");
        auto client = registerAndLogin(email, "Pass123", "Creator");

        Json::Value body;
        body["name"] = "My Project";
        body["description"] = "A test project";
        body["icon"] = "rocket";
        auto resp = client.post("/api/projects", body);

        CHECK(resp.statusCode == 201);
        CHECK(!resp.body["id"].asString().empty());
        CHECK(resp.body["success"].asBool() == true);
    }

    TEST_CASE("POST /api/projects - missing name returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("proj_noname");
        auto client = registerAndLogin(email, "Pass123", "User");

        Json::Value body;
        body["description"] = "no name";
        auto resp = client.post("/api/projects", body);
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Project name is required");
    }

    TEST_CASE("GET /api/projects - lists created project") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("proj_list");
        auto client = registerAndLogin(email, "Pass123", "Lister");

        Json::Value body;
        body["name"] = "Listed Project";
        client.post("/api/projects", body);

        auto resp = client.get("/api/projects");
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["projects"].size() == 1);
        CHECK(resp.body["projects"][0]["name"].asString() == "Listed Project");
        CHECK(resp.body["projects"][0].isMember("owner_id"));
        CHECK(resp.body["projects"][0].isMember("task_count"));
        CHECK(resp.body["projects"][0].isMember("member_count"));
    }

    TEST_CASE("GET /api/projects/{id} - returns full project details") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("proj_detail");
        auto client = registerAndLogin(email, "Pass123", "Detail User");

        auto projectId = createProject(client, "Detail Project");
        auto resp = client.get("/api/projects/" + projectId);

        CHECK(resp.statusCode == 200);
        CHECK(resp.body.isMember("project"));
        CHECK(resp.body.isMember("columns"));
        CHECK(resp.body.isMember("members"));
        CHECK(resp.body["project"]["name"].asString() == "Detail Project");
        // Default columns: "To Do" and "Done"
        CHECK(resp.body["columns"].size() == 2);
        // Owner should be a member
        CHECK(resp.body["members"].size() == 1);
        CHECK(resp.body["members"][0]["email"].asString() == email);
    }

    TEST_CASE("GET /api/projects/{id} - non-existent returns 404") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("proj_404");
        auto client = registerAndLogin(email, "Pass123", "User");

        auto resp = client.get("/api/projects/00000000-0000-0000-0000-000000000000");
        CHECK(resp.statusCode == 404);
        CHECK(resp.body["error"].asString() == "Project not found");
    }

    TEST_CASE("DELETE /api/projects/{id} - owner can delete") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("proj_del");
        auto client = registerAndLogin(email, "Pass123", "Deleter");

        auto projectId = createProject(client, "Deletable");
        auto resp = client.del("/api/projects/" + projectId);

        CHECK(resp.statusCode == 200);
        CHECK(resp.body["success"].asBool() == true);

        // Verify project is gone
        auto listResp = client.get("/api/projects");
        CHECK(listResp.body["projects"].size() == 0);
    }

    TEST_CASE("DELETE /api/projects/{id} - non-owner gets 403") {
        getTestDb().cleanAll();
        auto ownerEmail = uniqueEmail("proj_own");
        auto memberEmail = uniqueEmail("proj_mem");

        // Owner creates project
        auto ownerClient = registerAndLogin(ownerEmail, "Pass123", "Owner");
        auto projectId = createProject(ownerClient, "Owned Project");

        // Register second user
        auto memberClient = registerAndLogin(memberEmail, "Pass123", "Member");

        // Invite member
        Json::Value invite;
        invite["email"] = memberEmail;
        ownerClient.post("/api/projects/" + projectId + "/invite", invite);

        // Member tries to delete
        auto resp = memberClient.del("/api/projects/" + projectId);
        CHECK(resp.statusCode == 403);
        CHECK(resp.body["error"].asString() == "Only the project owner can delete this project");
    }

    TEST_CASE("POST /api/projects/{id}/invite - invites existing user") {
        getTestDb().cleanAll();
        auto ownerEmail = uniqueEmail("inv_own");
        auto inviteeEmail = uniqueEmail("inv_target");

        auto ownerClient = registerAndLogin(ownerEmail, "Pass123", "Owner");
        auto projectId = createProject(ownerClient, "Team Project");

        // Register invitee
        registerAndLogin(inviteeEmail, "Pass123", "Invitee");

        // Invite
        Json::Value body;
        body["email"] = inviteeEmail;
        auto resp = ownerClient.post("/api/projects/" + projectId + "/invite", body);
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["success"].asBool() == true);

        // Verify member count
        auto detailResp = ownerClient.get("/api/projects/" + projectId);
        CHECK(detailResp.body["members"].size() == 2);
    }

    TEST_CASE("POST /api/projects/{id}/invite - non-existent email returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("inv_bad");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Project");

        Json::Value body;
        body["email"] = "nobody@nowhere.com";
        auto resp = client.post("/api/projects/" + projectId + "/invite", body);
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString().find("not found") != std::string::npos);
    }

    TEST_CASE("POST /api/projects/{id}/invite - missing email returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("inv_noemail");
        auto client = registerAndLogin(email, "Pass123", "User");
        auto projectId = createProject(client, "Project");

        Json::Value body(Json::objectValue);
        auto resp = client.post("/api/projects/" + projectId + "/invite", body);
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Email is required");
    }

}
