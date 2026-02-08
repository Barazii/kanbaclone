#include "http_test_client.h"
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <unistd.h>

namespace httptest {

bool HttpResponse::hasHeader(const std::string& key) const {
    std::string lower = key;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return headers.find(lower) != headers.end();
}

std::string HttpResponse::getHeader(const std::string& key) const {
    std::string lower = key;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = headers.find(lower);
    if (it != headers.end()) return it->second;
    return "";
}

HttpTestClient::HttpTestClient(const std::string& baseUrl) {
    if (!baseUrl.empty()) {
        baseUrl_ = baseUrl;
    } else {
        const char* env = std::getenv("API_BASE_URL");
        baseUrl_ = env ? env : "http://localhost:3001";
    }

    // Create a unique temp file for the cookie jar
    char tmpl[] = "/tmp/httptest_cookies_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) {
        throw std::runtime_error("Failed to create cookie jar temp file");
    }
    close(fd);
    cookieJarPath_ = tmpl;
}

HttpTestClient::~HttpTestClient() {
    if (!cookieJarPath_.empty()) {
        std::remove(cookieJarPath_.c_str());
    }
}

HttpTestClient::HttpTestClient(HttpTestClient&& other) noexcept
    : baseUrl_(std::move(other.baseUrl_)),
      cookieJarPath_(std::move(other.cookieJarPath_)),
      origin_(std::move(other.origin_)) {
    other.cookieJarPath_.clear();  // Prevent double-delete of temp file
}

HttpTestClient& HttpTestClient::operator=(HttpTestClient&& other) noexcept {
    if (this != &other) {
        if (!cookieJarPath_.empty()) {
            std::remove(cookieJarPath_.c_str());
        }
        baseUrl_ = std::move(other.baseUrl_);
        cookieJarPath_ = std::move(other.cookieJarPath_);
        origin_ = std::move(other.origin_);
        other.cookieJarPath_.clear();
    }
    return *this;
}

HttpResponse HttpTestClient::get(const std::string& path) {
    return execute("GET", path);
}

HttpResponse HttpTestClient::post(const std::string& path, const Json::Value& body) {
    if (body.isNull()) return execute("POST", path);
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return execute("POST", path, Json::writeString(writer, body));
}

HttpResponse HttpTestClient::put(const std::string& path, const Json::Value& body) {
    if (body.isNull()) return execute("PUT", path);
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return execute("PUT", path, Json::writeString(writer, body));
}

HttpResponse HttpTestClient::del(const std::string& path) {
    return execute("DELETE", path);
}

HttpResponse HttpTestClient::options(const std::string& path) {
    return execute("OPTIONS", path);
}

void HttpTestClient::clearCookies() {
    // Truncate the cookie jar file
    FILE* f = fopen(cookieJarPath_.c_str(), "w");
    if (f) fclose(f);
}

void HttpTestClient::setOrigin(const std::string& origin) {
    origin_ = origin;
}

size_t HttpTestClient::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t HttpTestClient::headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t totalSize = size * nitems;
    auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);

    std::string line(buffer, totalSize);
    // Remove trailing \r\n
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }

    auto colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        // Trim leading whitespace from value
        auto start = value.find_first_not_of(" \t");
        if (start != std::string::npos) {
            value = value.substr(start);
        }

        // Lowercase the key
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // For set-cookie and other multi-value headers, append with newline
        auto it = headers->find(key);
        if (it != headers->end()) {
            it->second += "\n" + value;
        } else {
            (*headers)[key] = value;
        }
    }

    return totalSize;
}

HttpResponse HttpTestClient::execute(const std::string& method,
                                     const std::string& path,
                                     const std::string& requestBody) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to init curl");
    }

    std::string url = baseUrl_ + path;
    std::string responseBody;
    std::map<std::string, std::string> responseHeaders;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieJarPath_.c_str());
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookieJarPath_.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    struct curl_slist* headerList = nullptr;
    headerList = curl_slist_append(headerList, "Content-Type: application/json");

    if (!origin_.empty()) {
        headerList = curl_slist_append(headerList, ("Origin: " + origin_).c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)requestBody.size());
    } else if (method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)requestBody.size());
    } else if (method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (method == "OPTIONS") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    }
    // GET is the default

    CURLcode res = curl_easy_perform(curl);

    HttpResponse response;
    if (res != CURLE_OK) {
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);
    response.rawBody = responseBody;
    response.headers = responseHeaders;

    // Parse JSON body if content-type is json
    std::string contentType = response.getHeader("content-type");
    if (contentType.find("application/json") != std::string::npos && !responseBody.empty()) {
        Json::CharReaderBuilder reader;
        std::string errors;
        std::istringstream stream(responseBody);
        Json::parseFromStream(reader, stream, &response.body, &errors);
    }

    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);
    return response;
}

} // namespace httptest
