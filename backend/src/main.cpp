#include <drogon/drogon.h>
#include <cstdlib>
#include <iostream>
#include "utils/Database.h"
#include "utils/PasswordHash.h"

using namespace drogon;

int main() {
    // Initialize libsodium
    if (!kanba::utils::PasswordHash::initialize()) {
        LOG_FATAL << "Failed to initialize libsodium";
        return 1;
    }

    // Load environment variables for database configuration
    const char* dbHost = std::getenv("DATABASE_HOST");
    const char* dbPort = std::getenv("DATABASE_PORT");
    const char* dbName = std::getenv("DATABASE_NAME");
    const char* dbUser = std::getenv("DATABASE_USER");
    const char* dbPassword = std::getenv("DATABASE_PASSWORD");
    const char* port = std::getenv("PORT");

    // Set defaults
    if (!dbHost) dbHost = "localhost";
    if (!dbPort) dbPort = "5432";
    if (!dbName) dbName = "kanba";
    if (!dbUser) dbUser = "postgres";
    if (!dbPassword) dbPassword = "postgres";
    if (!port) port = "3001";

    std::cout << "Starting Kanba Backend (C++/Drogon)..." << std::endl;
    std::cout << "Database: " << dbHost << ":" << dbPort << "/" << dbName << std::endl;
    std::cout << "Port: " << port << std::endl;

    // Configure database connection
    app().createDbClient(
        "postgresql",           // rdbms
        dbHost,                 // host
        static_cast<unsigned short>(std::stoi(dbPort)),  // port
        dbName,                 // dbname
        dbUser,                 // user
        dbPassword,             // password
        10,                     // connection number
        "",                     // filename (for sqlite)
        "default",              // name
        false,                  // is_fast
        "",                     // characterSet
        30.0,                   // timeout
        false                   // autoBatch
    );

    // Handle CORS preflight OPTIONS requests before routing
    app().registerPreRoutingAdvice(
        [](const drogon::HttpRequestPtr& req,
           drogon::AdviceCallback&& acb,
           drogon::AdviceChainCallback&& accb) {
            const char* frontendUrl = std::getenv("FRONTEND_URL");
            std::string origin = frontendUrl ? frontendUrl : "http://localhost:5173";

            if (req->method() == drogon::Options) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k204NoContent);
                resp->addHeader("Access-Control-Allow-Origin", origin);
                resp->addHeader("Access-Control-Allow-Credentials", "true");
                resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                acb(resp);
                return;
            }
            accb();
        }
    );

    // Add CORS headers to all actual responses
    app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp) {
            const char* frontendUrl = std::getenv("FRONTEND_URL");
            std::string origin = frontendUrl ? frontendUrl : "http://localhost:5173";
            resp->addHeader("Access-Control-Allow-Origin", origin);
            resp->addHeader("Access-Control-Allow-Credentials", "true");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "86400");
        }
    );

    // Configure app settings
    app().setLogLevel(trantor::Logger::kInfo);
    app().addListener("0.0.0.0", static_cast<uint16_t>(std::stoi(port)));
    app().setThreadNum(4);

    // Log startup
    LOG_INFO << "Kanba C++ Backend starting on port " << port;

    // Run the application
    app().run();

    return 0;
}
