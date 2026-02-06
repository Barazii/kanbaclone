// =============================================================================
// Unit Tests: HealthController
//
// Tests the GET /api/health endpoint.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "controllers/HealthController.h"

using kanba::controllers::HealthController;
using namespace drogon;

TEST_SUITE("HealthController") {

TEST_CASE("health endpoint should return 200 with status ok") {
    HealthController controller;
    auto req = HttpRequest::newHttpRequest();

    HttpResponsePtr capturedResp;
    controller.health(req, [&](const HttpResponsePtr& resp) {
        capturedResp = resp;
    });

    REQUIRE(capturedResp != nullptr);
    CHECK(capturedResp->statusCode() == k200OK);
    auto json = capturedResp->getJsonObject();
    REQUIRE(json != nullptr);
    CHECK((*json)["status"].asString() == "ok");
}

} // TEST_SUITE
