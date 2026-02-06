// =============================================================================
// Unit Tests: CorsFilter
//
// Tests CORS header injection and OPTIONS preflight handling.
// =============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "filters/CorsFilter.h"

using kanba::filters::CorsFilter;
using namespace drogon;

TEST_SUITE("CorsFilter") {

// ---------------------------------------------------------------------------
// OPTIONS preflight requests
// ---------------------------------------------------------------------------

TEST_CASE("should respond to OPTIONS requests with 204 No Content") {
    CorsFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Options);

    HttpResponsePtr capturedResp;
    bool chainCalled = false;

    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        [&]() { chainCalled = true; }
    );

    REQUIRE(capturedResp != nullptr);
    CHECK(capturedResp->statusCode() == k204NoContent);
    CHECK(chainCalled == false);
}

TEST_CASE("OPTIONS response should include Access-Control-Allow-Origin header") {
    CorsFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Options);

    HttpResponsePtr capturedResp;
    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        []() {}
    );

    REQUIRE(capturedResp != nullptr);
    std::string origin = capturedResp->getHeader("Access-Control-Allow-Origin");
    CHECK_FALSE(origin.empty());
}

TEST_CASE("OPTIONS response should include Access-Control-Allow-Methods") {
    CorsFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Options);

    HttpResponsePtr capturedResp;
    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        []() {}
    );

    std::string methods = capturedResp->getHeader("Access-Control-Allow-Methods");
    CHECK(methods.find("GET") != std::string::npos);
    CHECK(methods.find("POST") != std::string::npos);
    CHECK(methods.find("PUT") != std::string::npos);
    CHECK(methods.find("DELETE") != std::string::npos);
    CHECK(methods.find("OPTIONS") != std::string::npos);
}

TEST_CASE("OPTIONS response should include Access-Control-Allow-Headers") {
    CorsFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Options);

    HttpResponsePtr capturedResp;
    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        []() {}
    );

    std::string headers = capturedResp->getHeader("Access-Control-Allow-Headers");
    CHECK(headers.find("Content-Type") != std::string::npos);
    CHECK(headers.find("Authorization") != std::string::npos);
}

TEST_CASE("OPTIONS response should include Access-Control-Allow-Credentials as true") {
    CorsFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Options);

    HttpResponsePtr capturedResp;
    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        []() {}
    );

    CHECK(capturedResp->getHeader("Access-Control-Allow-Credentials") == "true");
}

TEST_CASE("OPTIONS response should include Access-Control-Max-Age") {
    CorsFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Options);

    HttpResponsePtr capturedResp;
    filter.doFilter(
        req,
        [&](const HttpResponsePtr& resp) { capturedResp = resp; },
        []() {}
    );

    CHECK(capturedResp->getHeader("Access-Control-Max-Age") == "86400");
}

// ---------------------------------------------------------------------------
// Non-OPTIONS requests should pass through
// ---------------------------------------------------------------------------

TEST_CASE("non-OPTIONS request should call filter chain callback, not filter callback") {
    CorsFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Get);

    bool filterCalled = false;
    bool chainCalled = false;

    filter.doFilter(
        req,
        [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { chainCalled = true; }
    );

    CHECK(filterCalled == false);
    CHECK(chainCalled == true);
}

} // TEST_SUITE
