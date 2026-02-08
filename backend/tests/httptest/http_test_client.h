#pragma once

#include <string>
#include <map>
#include <json/json.h>
#include <curl/curl.h>

namespace httptest {

struct HttpResponse {
    long statusCode = 0;
    Json::Value body;
    std::string rawBody;
    std::map<std::string, std::string> headers;  // lowercase keys

    bool hasHeader(const std::string& key) const;
    std::string getHeader(const std::string& key) const;
};

class HttpTestClient {
public:
    explicit HttpTestClient(const std::string& baseUrl = "");
    ~HttpTestClient();

    HttpResponse get(const std::string& path);
    HttpResponse post(const std::string& path, const Json::Value& body = Json::nullValue);
    HttpResponse put(const std::string& path, const Json::Value& body = Json::nullValue);
    HttpResponse del(const std::string& path);
    HttpResponse options(const std::string& path);

    void clearCookies();
    void setOrigin(const std::string& origin);

    HttpTestClient(const HttpTestClient&) = delete;
    HttpTestClient& operator=(const HttpTestClient&) = delete;
    HttpTestClient(HttpTestClient&& other) noexcept;
    HttpTestClient& operator=(HttpTestClient&& other) noexcept;

private:
    std::string baseUrl_;
    std::string cookieJarPath_;
    std::string origin_;

    HttpResponse execute(const std::string& method,
                         const std::string& path,
                         const std::string& requestBody = "");

    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata);
};

} // namespace httptest
