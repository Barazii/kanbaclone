#include "ColumnController.h"
#include "../utils/Database.h"
#include "../filters/AuthFilter.h"

namespace kanba {
namespace controllers {

void ColumnController::createColumn(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("project_id") || !json->isMember("name")) {
        Json::Value error;
        error["error"] = "Project ID and name are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string projectId = (*json)["project_id"].asString();
    std::string name = (*json)["name"].asString();
    std::string color = json->isMember("color") ? (*json)["color"].asString() : "";

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM create_column($1, $2, $3)",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value error;
                error["error"] = "Failed to create column";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
                return;
            }

            auto row = result[0];
            Json::Value column;
            column["id"] = row["id"].as<std::string>();
            column["project_id"] = row["project_id"].as<std::string>();
            column["name"] = row["name"].as<std::string>();
            if (!row["color"].isNull()) {
                column["color"] = row["color"].as<std::string>();
            }
            column["position"] = row["position"].as<int>();

            auto resp = drogon::HttpResponse::newHttpJsonResponse(column);
            resp->setStatusCode(drogon::k201Created);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value error;
            error["error"] = "Database error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        projectId,
        name,
        color
    );
}

void ColumnController::updateColumn(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("id")) {
        Json::Value error;
        error["error"] = "Column ID is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string id = (*json)["id"].asString();
    std::string name = json->isMember("name") ? (*json)["name"].asString() : "";
    std::string color = json->isMember("color") ? (*json)["color"].asString() : "";

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM update_column($1, $2, $3)",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value error;
                error["error"] = "Column not found";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            auto row = result[0];
            Json::Value column;
            column["id"] = row["id"].as<std::string>();
            column["name"] = row["name"].as<std::string>();
            if (!row["color"].isNull()) {
                column["color"] = row["color"].as<std::string>();
            }
            column["position"] = row["position"].as<int>();

            auto resp = drogon::HttpResponse::newHttpJsonResponse(column);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value error;
            error["error"] = "Database error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        id,
        name,
        color
    );
}

void ColumnController::deleteColumn(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    std::string id = req->getParameter("id");

    if (id.empty()) {
        Json::Value error;
        error["error"] = "Column ID is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM delete_column($1)",
        [callback](const drogon::orm::Result& result) {
            Json::Value response;
            response["success"] = true;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value error;
            error["error"] = "Database error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        id
    );
}

} // namespace controllers
} // namespace kanba
