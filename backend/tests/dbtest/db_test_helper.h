#pragma once

#include <libpq-fe.h>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <set>

// RAII wrapper for PGresult
class PGResultGuard {
public:
    explicit PGResultGuard(PGresult* r) : res_(r) {}
    ~PGResultGuard() { if (res_) PQclear(res_); }
    PGresult* get() const { return res_; }
    operator PGresult*() const { return res_; }
    int ntuples() const { return PQntuples(res_); }
    int nfields() const { return PQnfields(res_); }
    int col(const char* name) const { return PQfnumber(res_, name); }
    std::string val(int row, const char* name) const {
        int c = PQfnumber(res_, name);
        if (c < 0) throw std::runtime_error(std::string("No column: ") + name);
        return PQgetvalue(res_, row, c);
    }
    bool isNull(int row, const char* name) const {
        int c = PQfnumber(res_, name);
        if (c < 0) return true;
        return PQgetisnull(res_, row, c);
    }
    PGResultGuard(const PGResultGuard&) = delete;
    PGResultGuard& operator=(const PGResultGuard&) = delete;
private:
    PGresult* res_;
};

// RAII connection wrapper with helpers
class TestDb {
public:
    TestDb() {
        const char* conninfo = std::getenv("TEST_DB_CONNINFO");
        if (!conninfo)
            conninfo = "host=localhost port=5433 dbname=kanba_test user=postgres password=testpassword";
        conn_ = PQconnectdb(conninfo);
        if (PQstatus(conn_) != CONNECTION_OK) {
            std::string err = PQerrorMessage(conn_);
            PQfinish(conn_);
            throw std::runtime_error("DB connection failed: " + err);
        }
    }

    ~TestDb() { if (conn_) PQfinish(conn_); }

    PGconn* get() const { return conn_; }

    PGresult* exec(const char* sql) {
        PGresult* res = PQexec(conn_, sql);
        ExecStatusType st = PQresultStatus(res);
        if (st != PGRES_TUPLES_OK && st != PGRES_COMMAND_OK) {
            std::string err = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error(std::string("SQL error: ") + err + " [" + sql + "]");
        }
        return res;
    }

    PGresult* execParams(const char* sql, int n, const char* const* vals) {
        PGresult* res = PQexecParams(conn_, sql, n, nullptr, vals, nullptr, nullptr, 0);
        ExecStatusType st = PQresultStatus(res);
        if (st != PGRES_TUPLES_OK && st != PGRES_COMMAND_OK) {
            std::string err = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error(std::string("SQL error: ") + err + " [" + sql + "]");
        }
        return res;
    }

    void cleanAll() {
        PQexec(conn_, "DELETE FROM activity_log");
        PQexec(conn_, "DELETE FROM task_comments");
        PQexec(conn_, "DELETE FROM tasks");
        PQexec(conn_, "DELETE FROM columns");
        PQexec(conn_, "DELETE FROM project_members");
        PQexec(conn_, "DELETE FROM sessions");
        PQexec(conn_, "DELETE FROM projects");
        PQexec(conn_, "DELETE FROM users");
    }

    std::string createTestUser(const char* email = "test@example.com",
                               const char* name = "Test User") {
        const char* p[] = { email, "$argon2id$fakehash", name };
        PGResultGuard res(execParams("SELECT * FROM create_user($1, $2, $3)", 3, p));
        return res.val(0, "id");
    }

    std::string createTestProject(const std::string& userId,
                                  const char* name = "Test Project") {
        const char* p[] = { name, "description", "", userId.c_str() };
        PGResultGuard res(execParams("SELECT create_project($1, $2, $3, $4) AS id", 4, p));
        return res.val(0, "id");
    }

    std::string getFirstColumnId(const std::string& projectId) {
        const char* p[] = { projectId.c_str() };
        PGResultGuard res(execParams("SELECT * FROM get_project_columns($1)", 1, p));
        return res.val(0, "id");
    }

    TestDb(const TestDb&) = delete;
    TestDb& operator=(const TestDb&) = delete;
private:
    PGconn* conn_;
};
