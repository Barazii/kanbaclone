#include "TaskController.h"
#include "../utils/Database.h"
#include "../filters/AuthFilter.h"

namespace kanba {
namespace controllers {

void TaskController::createTask(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("column_id") || !json->isMember("title")) {
        Json::Value error;
        error["error"] = "Column ID and title are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);
    std::string columnId = (*json)["column_id"].asString();
    std::string title = (*json)["title"].asString();
    std::string description = json->isMember("description") ? (*json)["description"].asString() : "";
    std::string priority = json->isMember("priority") ? (*json)["priority"].asString() : "medium";
    std::string assigneeId = json->isMember("assignee_id") ? (*json)["assignee_id"].asString() : "";
    std::string dueDate = json->isMember("due_date") ? (*json)["due_date"].asString() : "";

    // Handle tags as JSON array
    std::string tagsJson = "[]";
    if (json->isMember("tags") && (*json)["tags"].isArray()) {
        Json::StreamWriterBuilder writer;
        tagsJson = Json::writeString(writer, (*json)["tags"]);
    }

    auto db = utils::Database::getClient();

    // Use NULLIF to convert empty strings to NULL (avoids nullptr crash in Drogon)
    db->execSqlAsync(
        "SELECT * FROM create_task("
        "$1::uuid, $2, $3, $4, "
        "NULLIF($5,'')::uuid, "
        "NULLIF($6,'')::timestamptz, "
        "$7::jsonb, $8::uuid)",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value error;
                error["error"] = "Failed to create task";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
                return;
            }

            auto row = result[0];
            Json::Value task;
            task["id"] = row["id"].as<std::string>();
            task["column_id"] = row["column_id"].as<std::string>();
            task["title"] = row["title"].as<std::string>();
            if (!row["description"].isNull()) {
                task["description"] = row["description"].as<std::string>();
            }
            task["priority"] = row["priority"].as<std::string>();
            task["position"] = row["position"].as<int>();
            if (!row["assignee_id"].isNull()) {
                task["assignee_id"] = row["assignee_id"].as<std::string>();
            }
            if (!row["due_date"].isNull()) {
                task["due_date"] = row["due_date"].as<std::string>();
            }
            if (!row["tags"].isNull()) {
                Json::Reader reader;
                Json::Value tagsArray;
                if (reader.parse(row["tags"].as<std::string>(), tagsArray)) {
                    task["tags"] = tagsArray;
                }
            }
            task["created_at"] = row["created_at"].as<std::string>();

            auto resp = drogon::HttpResponse::newHttpJsonResponse(task);
            resp->setStatusCode(drogon::k201Created);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Create task error: " << e.base().what();
            Json::Value error;
            error["error"] = "Database error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        columnId,
        title,
        description,
        priority,
        assigneeId,
        dueDate,
        tagsJson,
        userId
    );
}

void TaskController::updateTask(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("id")) {
        Json::Value error;
        error["error"] = "Task ID is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);
    std::string id = (*json)["id"].asString();
    std::string title = json->isMember("title") ? (*json)["title"].asString() : "";
    std::string description = json->isMember("description") ? (*json)["description"].asString() : "";
    std::string priority = json->isMember("priority") ? (*json)["priority"].asString() : "";
    std::string assigneeId = json->isMember("assignee_id") ? (*json)["assignee_id"].asString() : "";
    std::string dueDate = json->isMember("due_date") ? (*json)["due_date"].asString() : "";

    std::string tagsJson = "null";
    if (json->isMember("tags") && (*json)["tags"].isArray()) {
        Json::StreamWriterBuilder writer;
        tagsJson = Json::writeString(writer, (*json)["tags"]);
    }

    auto db = utils::Database::getClient();

    // Use NULLIF to convert empty strings to NULL (avoids nullptr crash in Drogon)
    db->execSqlAsync(
        "SELECT * FROM update_task("
        "$1::uuid, "
        "NULLIF($2,''), NULLIF($3,''), NULLIF($4,''), "
        "NULLIF($5,'')::uuid, "
        "NULLIF($6,'')::timestamptz, "
        "NULLIF($7,'null')::jsonb, $8::uuid)",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value error;
                error["error"] = "Task not found";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            auto row = result[0];
            Json::Value task;
            task["id"] = row["id"].as<std::string>();
            task["column_id"] = row["column_id"].as<std::string>();
            task["title"] = row["title"].as<std::string>();
            if (!row["description"].isNull()) {
                task["description"] = row["description"].as<std::string>();
            }
            task["priority"] = row["priority"].as<std::string>();
            task["position"] = row["position"].as<int>();
            if (!row["assignee_id"].isNull()) {
                task["assignee_id"] = row["assignee_id"].as<std::string>();
            }
            if (!row["due_date"].isNull()) {
                task["due_date"] = row["due_date"].as<std::string>();
            }

            auto resp = drogon::HttpResponse::newHttpJsonResponse(task);
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
        title,
        description,
        priority,
        assigneeId,
        dueDate,
        tagsJson,
        userId
    );
}

void TaskController::deleteTask(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    std::string id = req->getParameter("id");
    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);

    if (id.empty()) {
        Json::Value error;
        error["error"] = "Task ID is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM delete_task($1::uuid, $2::uuid)",
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
        id,
        userId
    );
}

void TaskController::moveTask(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("task_id") || !json->isMember("column_id")) {
        Json::Value error;
        error["error"] = "Task ID and column ID are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);
    std::string taskId = (*json)["task_id"].asString();
    std::string columnId = (*json)["column_id"].asString();
    int position = json->isMember("position") ? (*json)["position"].asInt() : 0;

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM move_task($1::uuid, $2::uuid, $3, $4::uuid)",
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
        taskId,
        columnId,
        position,
        userId
    );
}

} // namespace controllers
} // namespace kanba
