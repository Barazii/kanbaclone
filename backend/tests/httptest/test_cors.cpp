#include "doctest.h"
#include "http_test_client.h"
#include "test_helpers.h"

TEST_SUITE("CORS") {

    TEST_CASE("OPTIONS request returns 204 with CORS headers") {
        httptest::HttpTestClient client;
        client.setOrigin("http://localhost:5173");
        auto resp = client.options("/api/health");

        CHECK(resp.statusCode == 204);
        CHECK(resp.getHeader("access-control-allow-origin") == "http://localhost:5173");
        CHECK(resp.getHeader("access-control-allow-credentials") == "true");
        CHECK(resp.getHeader("access-control-allow-methods").find("GET") != std::string::npos);
        CHECK(resp.getHeader("access-control-allow-methods").find("POST") != std::string::npos);
        CHECK(resp.getHeader("access-control-allow-methods").find("DELETE") != std::string::npos);
        CHECK(resp.getHeader("access-control-allow-headers").find("Content-Type") != std::string::npos);
        CHECK(resp.getHeader("access-control-max-age") == "86400");
    }

    TEST_CASE("OPTIONS on protected route returns 204 without 401") {
        httptest::HttpTestClient client;
        client.setOrigin("http://localhost:5173");
        auto resp = client.options("/api/projects");

        CHECK(resp.statusCode == 204);
        CHECK(resp.hasHeader("access-control-allow-origin"));
    }

    TEST_CASE("Regular GET response includes CORS headers") {
        httptest::HttpTestClient client;
        auto resp = client.get("/api/health");

        CHECK(resp.hasHeader("access-control-allow-origin"));
        CHECK(resp.getHeader("access-control-allow-credentials") == "true");
    }

    TEST_CASE("POST response includes CORS headers") {
        getTestDb().cleanAll();
        auto email = uniqueEmail("cors");
        auto client = registerAndLogin(email, "TestPass123", "CORS User");
        Json::Value body;
        body["name"] = "cors-test-project";
        auto resp = client.post("/api/projects", body);

        CHECK(resp.hasHeader("access-control-allow-origin"));
        CHECK(resp.getHeader("access-control-allow-credentials") == "true");
    }

}
