#include "CorsFilter.h"
#include <cstdlib>

namespace kanba {
namespace filters {

std::string CorsFilter::getFrontendUrl() const {
    const char* frontendUrl = std::getenv("FRONTEND_URL");
    return frontendUrl ? frontendUrl : "http://localhost:5173";
}

void CorsFilter::addCorsHeaders(drogon::HttpResponsePtr& resp) const {
    resp->addHeader("Access-Control-Allow-Origin", getFrontendUrl());
    resp->addHeader("Access-Control-Allow-Credentials", "true");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    resp->addHeader("Access-Control-Max-Age", "86400");
}

void CorsFilter::doFilter(
    const drogon::HttpRequestPtr& req,
    drogon::FilterCallback&& fcb,
    drogon::FilterChainCallback&& fccb
) {
    // Handle preflight OPTIONS request
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCorsHeaders(resp);
        resp->setStatusCode(drogon::k204NoContent);
        fcb(resp);
        return;
    }

    // For other requests, continue the chain but add CORS headers
    // We'll add headers in the response via a post-processing hook
    fccb();
}

} // namespace filters
} // namespace kanba
