#pragma once

#include <pqxx/pqxx>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <optional>

// Check if a result has a column by name
inline bool hasColumn(const pqxx::result& res, const char* name) {
    try {
        res.column_number(name);
        return true;
    } catch (const pqxx::argument_error&) {
        return false;
    }
}

// RAII connection wrapper with helpers
class TestDb {
public:
    TestDb() {
        const char* conninfo = std::getenv("TEST_DB_CONNINFO");
        if (!conninfo)
            conninfo = "host=localhost port=5433 dbname=kanba_test user=postgres password=testpassword";
        conn_ = std::make_unique<pqxx::connection>(conninfo);
    }

    pqxx::connection& conn() { return *conn_; }

    pqxx::result exec(const std::string& sql) {
        pqxx::nontransaction txn(*conn_);
        return txn.exec(sql);
    }

    template<typename... Args>
    pqxx::result execParams(const std::string& sql, Args&&... args) {
        pqxx::nontransaction txn(*conn_);
        return txn.exec_params(sql, std::forward<Args>(args)...);
    }

    void cleanAll() {
        pqxx::nontransaction txn(*conn_);
        txn.exec("DELETE FROM activity_log");
        txn.exec("DELETE FROM task_comments");
        txn.exec("DELETE FROM tasks");
        txn.exec("DELETE FROM columns");
        txn.exec("DELETE FROM project_members");
        txn.exec("DELETE FROM sessions");
        txn.exec("DELETE FROM projects");
        txn.exec("DELETE FROM users");
    }

    std::string createTestUser(const std::string& email = "test@example.com",
                               const std::string& name = "Test User") {
        pqxx::nontransaction txn(*conn_);
        auto res = txn.exec_params("SELECT * FROM create_user($1, $2, $3)",
                                    email, "$argon2id$fakehash", name);
        return res[0]["id"].as<std::string>();
    }

    std::string createTestProject(const std::string& userId,
                                  const std::string& name = "Test Project") {
        pqxx::nontransaction txn(*conn_);
        auto res = txn.exec_params("SELECT create_project($1, $2, $3, $4) AS id",
                                    name, "description", "", userId);
        return res[0]["id"].as<std::string>();
    }

    std::string getFirstColumnId(const std::string& projectId) {
        pqxx::nontransaction txn(*conn_);
        auto res = txn.exec_params("SELECT * FROM get_project_columns($1)", projectId);
        return res[0]["id"].as<std::string>();
    }

    TestDb(const TestDb&) = delete;
    TestDb& operator=(const TestDb&) = delete;

private:
    std::unique_ptr<pqxx::connection> conn_;
};
