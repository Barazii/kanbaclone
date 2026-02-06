#pragma once

#include <drogon/HttpFilter.h>

namespace kanba {
namespace filters {

class CorsFilter : public drogon::HttpFilter<CorsFilter> {
public:
    CorsFilter() = default;

    void doFilter(
        const drogon::HttpRequestPtr& req,
        drogon::FilterCallback&& fcb,
        drogon::FilterChainCallback&& fccb
    ) override;

private:
    std::string getFrontendUrl() const;
    void addCorsHeaders(drogon::HttpResponsePtr& resp) const;
};

} // namespace filters
} // namespace kanba
