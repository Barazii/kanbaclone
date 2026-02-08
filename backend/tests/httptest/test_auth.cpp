#include "doctest.h"
#include "http_test_client.h"
#include "test_helpers.h"

TEST_SUITE("Auth") {

    TEST_CASE("POST /api/auth/register - missing fields returns 400") {
        httptest::HttpTestClient client;

        SUBCASE("missing name") {
            Json::Value body;
            body["email"] = "a@b.com";
            body["password"] = "pw";
            auto resp = client.post("/api/auth/register", body);
            CHECK(resp.statusCode == 400);
        }

        SUBCASE("missing email") {
            Json::Value body;
            body["password"] = "pw";
            body["name"] = "X";
            auto resp = client.post("/api/auth/register", body);
            CHECK(resp.statusCode == 400);
        }

        SUBCASE("missing password") {
            Json::Value body;
            body["email"] = "a@b.com";
            body["name"] = "X";
            auto resp = client.post("/api/auth/register", body);
            CHECK(resp.statusCode == 400);
        }

        SUBCASE("empty body") {
            Json::Value body(Json::objectValue);
            auto resp = client.post("/api/auth/register", body);
            CHECK(resp.statusCode == 400);
        }
    }

    TEST_CASE("POST /api/auth/register - success") {
        getTestDb().cleanAll();
        httptest::HttpTestClient client;

        auto email = uniqueEmail("reg");
        Json::Value body;
        body["email"] = email;
        body["password"] = "SecurePass123";
        body["name"] = "New User";
        auto resp = client.post("/api/auth/register", body);

        CHECK(resp.statusCode == 200);
        CHECK(resp.body.isMember("user"));
        CHECK(resp.body["user"]["email"].asString() == email);
        CHECK(resp.body["user"]["name"].asString() == "New User");
        CHECK(!resp.body["user"]["id"].asString().empty());
        // Session cookie should be set (curl stores it in jar, but also visible in headers)
        CHECK(resp.getHeader("set-cookie").find("session=") != std::string::npos);
    }

    TEST_CASE("POST /api/auth/register - duplicate email returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("dup");

        // Register first user
        httptest::HttpTestClient client1;
        Json::Value body;
        body["email"] = email;
        body["password"] = "Pass123";
        body["name"] = "First";
        client1.post("/api/auth/register", body);

        // Try same email again
        httptest::HttpTestClient client2;
        auto resp = client2.post("/api/auth/register", body);
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Email already registered");
    }

    TEST_CASE("POST /api/auth/login - missing fields returns 400") {
        httptest::HttpTestClient client;
        Json::Value body(Json::objectValue);
        auto resp = client.post("/api/auth/login", body);
        CHECK(resp.statusCode == 400);
    }

    TEST_CASE("POST /api/auth/login - non-existent email returns 401") {
        httptest::HttpTestClient client;
        Json::Value body;
        body["email"] = "nobody@nowhere.com";
        body["password"] = "anything";
        auto resp = client.post("/api/auth/login", body);
        CHECK(resp.statusCode == 401);
        CHECK(resp.body["error"].asString() == "Invalid email or password");
    }

    TEST_CASE("POST /api/auth/login - wrong password returns 401") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("login");

        // Register
        httptest::HttpTestClient regClient;
        Json::Value regBody;
        regBody["email"] = email;
        regBody["password"] = "CorrectPass123";
        regBody["name"] = "Login Test";
        regClient.post("/api/auth/register", regBody);

        // Login with wrong password
        httptest::HttpTestClient client;
        Json::Value body;
        body["email"] = email;
        body["password"] = "WrongPass456";
        auto resp = client.post("/api/auth/login", body);
        CHECK(resp.statusCode == 401);
        CHECK(resp.body["error"].asString() == "Invalid email or password");
    }

    TEST_CASE("POST /api/auth/login - success with correct credentials") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("login_ok");

        // Register
        httptest::HttpTestClient regClient;
        Json::Value regBody;
        regBody["email"] = email;
        regBody["password"] = "CorrectPass123";
        regBody["name"] = "Login OK";
        regClient.post("/api/auth/register", regBody);

        // Login
        httptest::HttpTestClient client;
        Json::Value body;
        body["email"] = email;
        body["password"] = "CorrectPass123";
        auto resp = client.post("/api/auth/login", body);

        CHECK(resp.statusCode == 200);
        CHECK(resp.body.isMember("user"));
        CHECK(resp.body["user"]["email"].asString() == email);
        CHECK(resp.body["user"]["name"].asString() == "Login OK");
        CHECK(resp.getHeader("set-cookie").find("session=") != std::string::npos);
    }

    TEST_CASE("GET /api/auth/me - returns user when logged in") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("me");
        auto client = registerAndLogin(email, "Pass123", "Me User");

        auto resp = client.get("/api/auth/me");
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["user"]["email"].asString() == email);
        CHECK(resp.body["user"]["name"].asString() == "Me User");
    }

    TEST_CASE("GET /api/auth/me - returns null user when not logged in") {
        httptest::HttpTestClient client;
        auto resp = client.get("/api/auth/me");
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["user"].isNull());
    }

    TEST_CASE("POST /api/auth/logout - clears session") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("logout");
        auto client = registerAndLogin(email, "Pass123", "Logout User");

        // Logout
        auto resp = client.post("/api/auth/logout");
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["success"].asBool() == true);
        // Cookie should be cleared (Max-Age=0)
        auto setCookie = resp.getHeader("set-cookie");
        CHECK(setCookie.find("session=") != std::string::npos);
        CHECK(setCookie.find("Max-Age=0") != std::string::npos);

        // Verify: /me should now return null user
        auto meResp = client.get("/api/auth/me");
        CHECK(meResp.body["user"].isNull());
    }

    TEST_CASE("PUT /api/auth/update - requires authentication") {
        httptest::HttpTestClient client;
        Json::Value body;
        body["name"] = "Hacker";
        auto resp = client.put("/api/auth/update", body);
        CHECK(resp.statusCode == 401);
    }

    TEST_CASE("PUT /api/auth/update - updates name when authenticated") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("update");
        auto client = registerAndLogin(email, "Pass123", "Old Name");

        Json::Value body;
        body["name"] = "New Name";
        auto resp = client.put("/api/auth/update", body);
        CHECK(resp.statusCode == 200);
        CHECK(resp.body["user"]["name"].asString() == "New Name");

        // Verify via /me
        auto meResp = client.get("/api/auth/me");
        CHECK(meResp.body["user"]["name"].asString() == "New Name");
    }

    TEST_CASE("PUT /api/auth/update - missing name returns 400") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("update_bad");
        auto client = registerAndLogin(email, "Pass123", "User");

        Json::Value body(Json::objectValue);
        auto resp = client.put("/api/auth/update", body);
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Name is required");
    }

}
