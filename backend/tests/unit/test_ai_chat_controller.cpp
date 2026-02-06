// =============================================================================
// Unit Tests: AiChatController
//
// Tests the POST /api/ai-chat endpoint which proxies requests to OpenAI.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "controllers/AiChatController.h"

using kanba::controllers::AiChatController;
using namespace drogon;

static HttpRequestPtr makeJsonRequest(const Json::Value& json) {
    auto req = HttpRequest::newHttpRequest();
    req->setJsonBody(json);
    return req;
}

TEST_SUITE("AiChatController") {

TEST_CASE("should return 400 when no JSON body is provided") {
    AiChatController ctrl;
    auto req = HttpRequest::newHttpRequest();

    HttpResponsePtr resp;
    ctrl.chat(req, [&](const HttpResponsePtr& r) { resp = r; });

    REQUIRE(resp != nullptr);
    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Messages and API key are required");
}

TEST_CASE("should return 400 when messages field is missing") {
    AiChatController ctrl;
    Json::Value body;
    body["apiKey"] = "sk-test-key";
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.chat(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 400 when apiKey field is missing") {
    AiChatController ctrl;
    Json::Value body;
    Json::Value messages(Json::arrayValue);
    Json::Value msg;
    msg["role"] = "user";
    msg["content"] = "Hello";
    messages.append(msg);
    body["messages"] = messages;
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.chat(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
}

TEST_CASE("should return 400 when both fields are missing from empty object") {
    AiChatController ctrl;
    Json::Value body(Json::objectValue);
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.chat(req, [&](const HttpResponsePtr& r) { resp = r; });

    CHECK(resp->statusCode() == k400BadRequest);
    CHECK((*resp->getJsonObject())["error"].asString() == "Messages and API key are required");
}

// Note: Testing the actual OpenAI API call requires the mock HttpClient.
// The current mock HttpClient returns NetworkFailure by default, so the
// controller will return a 502 Bad Gateway.

TEST_CASE("should return 502 when OpenAI API connection fails") {
    AiChatController ctrl;
    Json::Value body;
    body["apiKey"] = "sk-test-key";
    Json::Value messages(Json::arrayValue);
    Json::Value msg;
    msg["role"] = "user";
    msg["content"] = "Hello";
    messages.append(msg);
    body["messages"] = messages;
    auto req = makeJsonRequest(body);

    HttpResponsePtr resp;
    ctrl.chat(req, [&](const HttpResponsePtr& r) { resp = r; });

    // With mock HttpClient that returns NetworkFailure:
    CHECK(resp->statusCode() == k502BadGateway);
    CHECK((*resp->getJsonObject())["error"].asString() == "Failed to connect to OpenAI API");
}

} // TEST_SUITE
