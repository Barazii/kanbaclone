#include "doctest.h"
#include "http_test_client.h"

TEST_SUITE("AI Chat") {

    TEST_CASE("POST /api/ai-chat - empty body returns 400") {
        httptest::HttpTestClient client;
        Json::Value body(Json::objectValue);
        auto resp = client.post("/api/ai-chat", body);
        CHECK(resp.statusCode == 400);
        CHECK(resp.body["error"].asString() == "Messages and API key are required");
    }

    TEST_CASE("POST /api/ai-chat - missing apiKey returns 400") {
        httptest::HttpTestClient client;
        Json::Value body;
        body["messages"] = Json::Value(Json::arrayValue);
        auto resp = client.post("/api/ai-chat", body);
        CHECK(resp.statusCode == 400);
    }

    TEST_CASE("POST /api/ai-chat - missing messages returns 400") {
        httptest::HttpTestClient client;
        Json::Value body;
        body["apiKey"] = "sk-test";
        auto resp = client.post("/api/ai-chat", body);
        CHECK(resp.statusCode == 400);
    }

}
