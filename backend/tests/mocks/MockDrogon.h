#pragma once
// =============================================================================
// MockDrogon.h - Lightweight mock layer for Drogon types
//
// Provides minimal stand-in types that mirror the Drogon API surface used by
// the kanba backend.  This allows unit tests to compile and run without
// linking the real Drogon library.
// =============================================================================

#include <json/json.h>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <vector>
#include <stdexcept>
#include <any>
#include <iostream>
#include <cstring>

// ---------------------------------------------------------------------------
// Trantor logger stub
// ---------------------------------------------------------------------------
namespace trantor {
class Logger {
public:
    enum LogLevel { kTrace, kDebug, kInfo, kWarn, kError, kFatal };
};
} // namespace trantor

// Macro stubs for LOG_*
struct NullStream {
    template <typename T>
    NullStream& operator<<(const T&) { return *this; }
};

#ifndef LOG_DEBUG
#define LOG_DEBUG NullStream()
#endif
#ifndef LOG_INFO
#define LOG_INFO NullStream()
#endif
#ifndef LOG_WARN
#define LOG_WARN NullStream()
#endif
#ifndef LOG_ERROR
#define LOG_ERROR NullStream()
#endif

// ---------------------------------------------------------------------------
// Drogon namespace mocks
// ---------------------------------------------------------------------------
namespace drogon {

enum HttpMethod { Get = 0, Post, Head, Put, Delete, Options, Patch, Invalid };

enum HttpStatusCode {
    k200OK = 200, k201Created = 201, k204NoContent = 204,
    k301MovedPermanently = 301, k400BadRequest = 400,
    k401Unauthorized = 401, k403Forbidden = 403, k404NotFound = 404,
    k500InternalServerError = 500, k502BadGateway = 502
};

enum ContentType { CT_APPLICATION_JSON, CT_TEXT_HTML, CT_TEXT_PLAIN };

enum class ReqResult {
    Ok = 0, BadResponse, NetworkFailure, BadServerAddress, Timeout, HandshakeError
};

// ---------------------------------------------------------------------------
// Attributes
// ---------------------------------------------------------------------------
class Attributes {
public:
    template <typename T>
    void insert(const std::string& key, const T& value) { data_[key] = value; }

    template <typename T>
    T get(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) return std::any_cast<T>(it->second);
        return T{};
    }

    bool has(const std::string& key) const { return data_.find(key) != data_.end(); }
private:
    std::map<std::string, std::any> data_;
};

using AttributesPtr = std::shared_ptr<Attributes>;

// ---------------------------------------------------------------------------
// Cookie
// ---------------------------------------------------------------------------
class Cookie {
public:
    struct SameSite {
        static constexpr int kLax = 0;
        static constexpr int kStrict = 1;
        static constexpr int kNone = 2;
    };
    Cookie() = default;
    Cookie(const std::string& name, const std::string& value) : name_(name), value_(value) {}
    void setHttpOnly(bool v) { httpOnly_ = v; }
    void setSecure(bool v) { secure_ = v; }
    void setPath(const std::string& p) { path_ = p; }
    void setSameSite(int s) { sameSite_ = s; }
    void setMaxAge(int a) { maxAge_ = a; }
    const std::string& key() const { return name_; }
    const std::string& value() const { return value_; }
    bool isHttpOnly() const { return httpOnly_; }
    bool isSecure() const { return secure_; }
    const std::string& path() const { return path_; }
    int sameSite() const { return sameSite_; }
    int maxAge() const { return maxAge_; }
private:
    std::string name_, value_, path_;
    bool httpOnly_ = false, secure_ = false;
    int sameSite_ = SameSite::kLax, maxAge_ = -1;
};

// ---------------------------------------------------------------------------
// HttpResponse (mock)
// ---------------------------------------------------------------------------
class HttpResponse;
using HttpResponsePtr = std::shared_ptr<HttpResponse>;

class HttpResponse {
public:
    HttpResponse() : jsonObj_(std::make_shared<Json::Value>()) {}

    static HttpResponsePtr newHttpResponse() { return std::make_shared<HttpResponse>(); }
    static HttpResponsePtr newHttpJsonResponse(const Json::Value& json) {
        auto resp = std::make_shared<HttpResponse>();
        *resp->jsonObj_ = json;
        return resp;
    }

    void setStatusCode(HttpStatusCode code) { statusCode_ = code; }
    HttpStatusCode statusCode() const { return statusCode_; }
    HttpStatusCode getStatusCode() const { return statusCode_; }

    void addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }
    std::string getHeader(const std::string& key) const {
        auto it = headers_.find(key);
        return it != headers_.end() ? it->second : "";
    }
    const std::map<std::string, std::string>& headers() const { return headers_; }

    void addCookie(const Cookie& cookie) { cookies_[cookie.key()] = cookie; }
    Cookie getCookie(const std::string& name) const {
        auto it = cookies_.find(name);
        return it != cookies_.end() ? it->second : Cookie{};
    }
    bool hasCookie(const std::string& name) const { return cookies_.find(name) != cookies_.end(); }

    std::shared_ptr<Json::Value> jsonObject() { return jsonObj_; }
    std::shared_ptr<Json::Value> getJsonObject() const { return jsonObj_; }

    void setBody(const std::string& body) { body_ = body; }
    const std::string& body() const { return body_; }
    void setContentTypeCode(ContentType ct) { contentType_ = ct; }
private:
    HttpStatusCode statusCode_ = k200OK;
    std::map<std::string, std::string> headers_;
    std::map<std::string, Cookie> cookies_;
    std::shared_ptr<Json::Value> jsonObj_;
    std::string body_;
    ContentType contentType_ = CT_APPLICATION_JSON;
};

// ---------------------------------------------------------------------------
// HttpRequest (mock)
// ---------------------------------------------------------------------------
class HttpRequest;
using HttpRequestPtr = std::shared_ptr<HttpRequest>;

class HttpRequest {
public:
    HttpRequest() : attrs_(std::make_shared<Attributes>()) {}
    static HttpRequestPtr newHttpRequest() { return std::make_shared<HttpRequest>(); }

    void setMethod(HttpMethod m) { method_ = m; }
    HttpMethod method() const { return method_; }

    void setPath(const std::string& p) { path_ = p; }
    const std::string& path() const { return path_; }

    void setJsonBody(const Json::Value& json) { jsonObj_ = std::make_shared<Json::Value>(json); }
    std::shared_ptr<Json::Value> getJsonObject() const { return jsonObj_; }

    void setCookie(const std::string& name, const std::string& value) { cookies_[name] = value; }
    std::string getCookie(const std::string& name) const {
        auto it = cookies_.find(name);
        return it != cookies_.end() ? it->second : "";
    }

    void setParameter(const std::string& key, const std::string& val) { params_[key] = val; }
    std::string getParameter(const std::string& key) const {
        auto it = params_.find(key);
        return it != params_.end() ? it->second : "";
    }

    AttributesPtr attributes() { return attrs_; }
    const AttributesPtr attributes() const { return attrs_; }

    void setBody(const std::string& body) { body_ = body; }
    const std::string& body() const { return body_; }
    void setContentTypeCode(ContentType ct) { contentType_ = ct; }
    void addHeader(const std::string& k, const std::string& v) { headers_[k] = v; }
    std::string getHeader(const std::string& k) const {
        auto it = headers_.find(k);
        return it != headers_.end() ? it->second : "";
    }
private:
    HttpMethod method_ = Get;
    std::string path_, body_;
    std::shared_ptr<Json::Value> jsonObj_;
    std::map<std::string, std::string> cookies_, params_, headers_;
    AttributesPtr attrs_;
    ContentType contentType_ = CT_APPLICATION_JSON;
};

// ---------------------------------------------------------------------------
// ORM mocks
// ---------------------------------------------------------------------------
namespace orm {

class DrogonDbException : public std::exception {
public:
    explicit DrogonDbException(const std::string& msg) : msg_(msg) {}
    const std::exception& base() const { return *this; }
    const char* what() const noexcept override { return msg_.c_str(); }
private:
    std::string msg_;
};

class Field {
public:
    Field() = default;
    explicit Field(const std::string& val, bool isNull = false) : value_(val), isNull_(isNull) {}

    template <typename T>
    T as() const {
        if constexpr (std::is_same_v<T, std::string>) { return value_; }
        else if constexpr (std::is_same_v<T, int>) { return std::stoi(value_); }
        else if constexpr (std::is_same_v<T, long>) { return std::stol(value_); }
        else if constexpr (std::is_same_v<T, double>) { return std::stod(value_); }
        else if constexpr (std::is_same_v<T, bool>) { return value_ == "true" || value_ == "1"; }
        return T{};
    }
    bool isNull() const { return isNull_; }
private:
    std::string value_;
    bool isNull_ = false;
};

class Row {
public:
    void addField(const std::string& name, const std::string& value, bool isNull = false) {
        fields_[name] = Field(value, isNull);
    }
    Field operator[](const std::string& name) const {
        auto it = fields_.find(name);
        if (it != fields_.end()) return it->second;
        return Field("", true);
    }
private:
    std::map<std::string, Field> fields_;
};

class Result {
public:
    Result() = default;
    void addRow(const Row& row) { rows_.push_back(row); }
    bool empty() const { return rows_.empty(); }
    size_t size() const { return rows_.size(); }
    size_t affectedRows() const { return affectedRows_; }
    void setAffectedRows(size_t n) { affectedRows_ = n; }
    const Row& operator[](size_t index) const { return rows_[index]; }
    auto begin() const { return rows_.begin(); }
    auto end() const { return rows_.end(); }
private:
    std::vector<Row> rows_;
    size_t affectedRows_ = 0;
};

class DbClient;
using DbClientPtr = std::shared_ptr<DbClient>;

class DbClient {
public:
    using ResultCallback = std::function<void(const Result&)>;
    using ErrorCallback = std::function<void(const DrogonDbException&)>;

    void setNextResult(const Result& result) { nextResult_ = result; shouldError_ = false; }
    void setNextError(const std::string& errorMsg) { nextError_ = errorMsg; shouldError_ = true; }
    void clearError() { shouldError_ = false; nextError_.clear(); }
    const std::string& lastSql() const { return lastSql_; }
    int callCount() const { return callCount_; }
    void reset() { callCount_ = 0; lastSql_.clear(); shouldError_ = false; nextResult_ = Result{}; }

    // Primary overload: sql, callback, errorCallback, args...
    template <typename... Args>
    void execSqlAsync(const std::string& sql, ResultCallback rcb, ErrorCallback ecb, Args&&...) {
        lastSql_ = sql;
        callCount_++;
        if (shouldError_) { shouldError_ = false; ecb(DrogonDbException(nextError_)); }
        else { rcb(nextResult_); }
    }
private:
    Result nextResult_;
    std::string nextError_;
    bool shouldError_ = false;
    std::string lastSql_;
    int callCount_ = 0;
};

} // namespace orm

// ---------------------------------------------------------------------------
// HttpClient mock
// ---------------------------------------------------------------------------
class HttpClient;
using HttpClientPtr = std::shared_ptr<HttpClient>;

class HttpClient {
public:
    using HttpReqCallback = std::function<void(ReqResult, const HttpResponsePtr&)>;
    static HttpClientPtr newHttpClient(const std::string&) { return std::make_shared<HttpClient>(); }
    void sendRequest(const HttpRequestPtr&, HttpReqCallback&& cb, double = 10.0) {
        if (nextResponse_) cb(ReqResult::Ok, nextResponse_);
        else cb(ReqResult::NetworkFailure, nullptr);
    }
    void setNextResponse(const HttpResponsePtr& resp) { nextResponse_ = resp; }
private:
    HttpResponsePtr nextResponse_;
};

using FilterCallback = std::function<void(const HttpResponsePtr&)>;
using FilterChainCallback = std::function<void()>;

// ---------------------------------------------------------------------------
// app() mock singleton
// ---------------------------------------------------------------------------
class MockApp {
public:
    static MockApp& instance() { static MockApp inst; return inst; }
    void setDbClient(const std::string& name, orm::DbClientPtr client) { dbClients_[name] = client; }
    orm::DbClientPtr getDbClient(const std::string& name = "default") {
        auto it = dbClients_.find(name);
        return it != dbClients_.end() ? it->second : nullptr;
    }
    void setLogLevel(trantor::Logger::LogLevel) {}
    void addListener(const std::string&, uint16_t) {}
    void setThreadNum(int) {}
    void enableSession(int = 0) {}
    void run() {}
    Json::Value getCustomConfig() { return config_; }
    Json::Value config_;
    void createDbClient(const std::string&, const std::string&, unsigned short,
        const std::string&, const std::string&, const std::string&,
        int, const std::string&, const std::string&,
        bool, const std::string& = "", double = 30.0, bool = false) {}
private:
    std::map<std::string, orm::DbClientPtr> dbClients_;
    MockApp() = default;
};

inline MockApp& app() { return MockApp::instance(); }

template <typename T> class HttpFilter {
public:
    virtual ~HttpFilter() = default;
    virtual void doFilter(const HttpRequestPtr&, FilterCallback&&, FilterChainCallback&&) = 0;
};

template <typename T> class HttpController {
public:
    virtual ~HttpController() = default;
};

#define METHOD_LIST_BEGIN
#define METHOD_LIST_END
#define ADD_METHOD_TO(...)

} // namespace drogon
