#pragma once

#include <drogon/HttpFilter.h>

namespace kanba {
namespace filters {

class AuthFilter : public drogon::HttpFilter<AuthFilter> {
public:
    AuthFilter() = default;

    void doFilter(
        const drogon::HttpRequestPtr& req,
        drogon::FilterCallback&& fcb,
        drogon::FilterChainCallback&& fccb
    ) override;

    // Key used to store user ID in request attributes
    static constexpr const char* USER_ID_KEY = "userId";
};

} // namespace filters
} // namespace kanba
