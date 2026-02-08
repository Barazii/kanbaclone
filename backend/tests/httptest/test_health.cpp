#include "doctest.h"
#include "http_test_client.h"

TEST_SUITE("Health") {

    TEST_CASE("GET /api/health returns 200 with status ok") {
        httptest::HttpTestClient client;
        auto resp = client.get("/api/health");

        CHECK(resp.statusCode == 200);
        CHECK(resp.body["status"].asString() == "ok");
    }

    TEST_CASE("GET /api/health returns JSON content type") {
        httptest::HttpTestClient client;
        auto resp = client.get("/api/health");

        CHECK(resp.hasHeader("content-type"));
        CHECK(resp.getHeader("content-type").find("application/json") != std::string::npos);
    }

}
