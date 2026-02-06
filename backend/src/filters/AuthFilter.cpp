#include "AuthFilter.h"
#include "../utils/Session.h"

namespace kanba {
namespace filters {

void AuthFilter::doFilter(
    const drogon::HttpRequestPtr& req,
    drogon::FilterCallback&& fcb,
    drogon::FilterChainCallback&& fccb
) {
    // Get session cookie
    std::string sessionId = req->getCookie(utils::Session::COOKIE_NAME);

    if (sessionId.empty()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value(Json::objectValue)
        );
        (*resp->jsonObject())["error"] = "Unauthorized";
        resp->setStatusCode(drogon::k401Unauthorized);
        fcb(resp);
        return;
    }

    // Validate session and get user ID
    utils::Session::getUserIdFromSession(
        sessionId,
        [req, fcb = std::move(fcb), fccb = std::move(fccb)](std::optional<std::string> userId) mutable {
            if (!userId.has_value()) {
                auto resp = drogon::HttpResponse::newHttpJsonResponse(
                    Json::Value(Json::objectValue)
                );
                (*resp->jsonObject())["error"] = "Unauthorized";
                resp->setStatusCode(drogon::k401Unauthorized);
                fcb(resp);
                return;
            }

            // Store user ID in request attributes for controllers to access
            req->attributes()->insert(USER_ID_KEY, *userId);

            // Continue to the handler
            fccb();
        }
    );
}

} // namespace filters
} // namespace kanba
