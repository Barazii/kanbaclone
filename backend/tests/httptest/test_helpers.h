#pragma once

#include "http_test_client.h"
#include "../dbtest/db_test_helper.h"
#include <string>
#include <vector>
#include <atomic>

// Global test DB for cleanup between tests
TestDb& getTestDb();

// Register a user via HTTP and return a logged-in client (session cookie in jar)
httptest::HttpTestClient registerAndLogin(
    const std::string& email = "test@example.com",
    const std::string& password = "TestPassword123",
    const std::string& name = "Test User"
);

// Generate unique email addresses to avoid conflicts
std::string uniqueEmail(const std::string& prefix = "test");

// Create a project via HTTP using an authenticated client, returns project ID
std::string createProject(httptest::HttpTestClient& client,
                          const std::string& name = "Test Project");

// Get the first column ID for a project via HTTP
std::string getFirstColumnId(httptest::HttpTestClient& client,
                             const std::string& projectId);

// Get column IDs for a project (returns vector of {id, name} pairs)
std::vector<std::pair<std::string, std::string>> getProjectColumns(
    httptest::HttpTestClient& client,
    const std::string& projectId);

// Create a task via HTTP, returns task ID
std::string createTask(httptest::HttpTestClient& client,
                       const std::string& columnId,
                       const std::string& title = "Test Task");
