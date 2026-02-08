#include "test_helpers.h"
#include <stdexcept>

static TestDb* gTestDb = nullptr;

TestDb& getTestDb() {
    if (!gTestDb) {
        gTestDb = new TestDb();
    }
    return *gTestDb;
}

void cleanupTestDb() {
    delete gTestDb;
    gTestDb = nullptr;
}

static std::atomic<int> emailCounter{0};

httptest::HttpTestClient registerAndLogin(
    const std::string& email,
    const std::string& password,
    const std::string& name
) {
    httptest::HttpTestClient client;
    Json::Value body;
    body["email"] = email;
    body["password"] = password;
    body["name"] = name;
    auto resp = client.post("/api/auth/register", body);
    if (resp.statusCode != 200) {
        throw std::runtime_error("registerAndLogin failed: HTTP " +
            std::to_string(resp.statusCode) + " - " + resp.rawBody);
    }
    return client;
}

std::string uniqueEmail(const std::string& prefix) {
    int n = emailCounter.fetch_add(1);
    return prefix + "_" + std::to_string(n) + "@test.com";
}

std::string createProject(httptest::HttpTestClient& client,
                          const std::string& name) {
    Json::Value body;
    body["name"] = name;
    auto resp = client.post("/api/projects", body);
    if (resp.statusCode != 201) {
        throw std::runtime_error("createProject failed: HTTP " +
            std::to_string(resp.statusCode) + " - " + resp.rawBody);
    }
    return resp.body["id"].asString();
}

std::string getFirstColumnId(httptest::HttpTestClient& client,
                             const std::string& projectId) {
    auto cols = getProjectColumns(client, projectId);
    if (cols.empty()) {
        throw std::runtime_error("No columns found for project " + projectId);
    }
    return cols[0].first;
}

std::vector<std::pair<std::string, std::string>> getProjectColumns(
    httptest::HttpTestClient& client,
    const std::string& projectId
) {
    auto resp = client.get("/api/projects/" + projectId);
    if (resp.statusCode != 200) {
        throw std::runtime_error("getProjectColumns failed: HTTP " +
            std::to_string(resp.statusCode));
    }
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& col : resp.body["columns"]) {
        result.emplace_back(col["id"].asString(), col["name"].asString());
    }
    return result;
}

std::string createTask(httptest::HttpTestClient& client,
                       const std::string& columnId,
                       const std::string& title) {
    Json::Value body;
    body["column_id"] = columnId;
    body["title"] = title;
    auto resp = client.post("/api/tasks", body);
    if (resp.statusCode != 201) {
        throw std::runtime_error("createTask failed: HTTP " +
            std::to_string(resp.statusCode) + " - " + resp.rawBody);
    }
    return resp.body["id"].asString();
}
