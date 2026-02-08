#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "test_helpers.h"
#include <iostream>

// Defined in test_helpers.cpp
void cleanupTestDb();

int main(int argc, char** argv) {
    doctest::Context context;
    context.applyCommandLine(argc, argv);

    // Verify DB connectivity before running tests
    try {
        getTestDb().cleanAll();
        std::cout << "Test database connected and cleaned." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to test database: " << e.what() << std::endl;
        return 1;
    }

    // Verify backend is reachable
    try {
        httptest::HttpTestClient client;
        auto resp = client.get("/api/health");
        if (resp.statusCode != 200) {
            std::cerr << "Backend health check failed: HTTP " << resp.statusCode << std::endl;
            return 1;
        }
        std::cout << "Backend is reachable at health endpoint." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to backend: " << e.what() << std::endl;
        return 1;
    }

    int res = context.run();

    cleanupTestDb();
    return res;
}
