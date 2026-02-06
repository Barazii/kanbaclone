#include "HealthController.h"

namespace kanba {
namespace controllers {

void HealthController::health(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    Json::Value result;
    result["status"] = "ok";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
    callback(resp);
}

} // namespace controllers
} // namespace kanba
