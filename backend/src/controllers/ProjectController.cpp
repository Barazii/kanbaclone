#include "ProjectController.h"
#include "../utils/Database.h"
#include "../filters/AuthFilter.h"

namespace kanba {
namespace controllers {

void ProjectController::getProjects(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM get_user_projects($1)",
        [callback](const drogon::orm::Result& result) {
            Json::Value projects(Json::arrayValue);

            for (const auto& row : result) {
                Json::Value project;
                project["id"] = row["id"].as<std::string>();
                project["name"] = row["name"].as<std::string>();
                if (!row["description"].isNull()) {
                    project["description"] = row["description"].as<std::string>();
                }
                if (!row["icon"].isNull()) {
                    project["icon"] = row["icon"].as<std::string>();
                }
                project["owner_id"] = row["owner_id"].as<std::string>();
                project["task_count"] = row["task_count"].as<int>();
                project["member_count"] = row["member_count"].as<int>();
                project["created_at"] = row["created_at"].as<std::string>();
                projects.append(project);
            }

            auto resp = drogon::HttpResponse::newHttpJsonResponse(projects);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value error;
            error["error"] = "Database error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        userId
    );
}

void ProjectController::createProject(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("name")) {
        Json::Value error;
        error["error"] = "Project name is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);
    std::string name = (*json)["name"].asString();
    std::string description = json->isMember("description") ? (*json)["description"].asString() : "";
    std::string icon = json->isMember("icon") ? (*json)["icon"].asString() : "";

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM create_project($1, $2, $3, $4)",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value error;
                error["error"] = "Failed to create project";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
                return;
            }

            Json::Value response;
            response["id"] = result[0]["id"].as<std::string>();
            response["success"] = true;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
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
        name,
        description,
        icon,
        userId
    );
}

void ProjectController::getProject(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
) {
    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);

    auto db = utils::Database::getClient();

    // Get project details
    db->execSqlAsync(
        "SELECT * FROM get_project_details($1)",
        [db, id, callback](const drogon::orm::Result& projectResult) {
            if (projectResult.empty()) {
                Json::Value error;
                error["error"] = "Project not found";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            auto projectRow = projectResult[0];
            Json::Value project;
            project["id"] = projectRow["id"].as<std::string>();
            project["name"] = projectRow["name"].as<std::string>();
            if (!projectRow["description"].isNull()) {
                project["description"] = projectRow["description"].as<std::string>();
            }
            if (!projectRow["icon"].isNull()) {
                project["icon"] = projectRow["icon"].as<std::string>();
            }
            project["owner_id"] = projectRow["owner_id"].as<std::string>();
            project["created_at"] = projectRow["created_at"].as<std::string>();

            // Get columns
            db->execSqlAsync(
                "SELECT * FROM get_project_columns($1)",
                [db, id, project, callback](const drogon::orm::Result& columnsResult) mutable {
                    Json::Value columns(Json::arrayValue);
                    for (const auto& row : columnsResult) {
                        Json::Value column;
                        column["id"] = row["id"].as<std::string>();
                        column["name"] = row["name"].as<std::string>();
                        if (!row["color"].isNull()) {
                            column["color"] = row["color"].as<std::string>();
                        }
                        column["position"] = row["position"].as<int>();
                        column["task_count"] = row["task_count"].as<int>();
                        columns.append(column);
                    }
                    project["columns"] = columns;

                    // Get tasks
                    db->execSqlAsync(
                        "SELECT * FROM get_project_tasks($1)",
                        [db, id, project, callback](const drogon::orm::Result& tasksResult) mutable {
                            Json::Value tasks(Json::arrayValue);
                            for (const auto& row : tasksResult) {
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
                                if (!row["assignee_name"].isNull()) {
                                    task["assignee_name"] = row["assignee_name"].as<std::string>();
                                }
                                if (!row["due_date"].isNull()) {
                                    task["due_date"] = row["due_date"].as<std::string>();
                                }
                                if (!row["tags"].isNull()) {
                                    // Parse tags JSON array
                                    Json::Reader reader;
                                    Json::Value tagsArray;
                                    if (reader.parse(row["tags"].as<std::string>(), tagsArray)) {
                                        task["tags"] = tagsArray;
                                    }
                                }
                                task["created_at"] = row["created_at"].as<std::string>();
                                tasks.append(task);
                            }
                            project["tasks"] = tasks;

                            // Get members
                            db->execSqlAsync(
                                "SELECT * FROM get_project_members($1)",
                                [project, callback](const drogon::orm::Result& membersResult) mutable {
                                    Json::Value members(Json::arrayValue);
                                    for (const auto& row : membersResult) {
                                        Json::Value member;
                                        member["id"] = row["id"].as<std::string>();
                                        member["name"] = row["name"].as<std::string>();
                                        member["email"] = row["email"].as<std::string>();
                                        member["role"] = row["role"].as<std::string>();
                                        if (!row["avatar_url"].isNull()) {
                                            member["avatar_url"] = row["avatar_url"].as<std::string>();
                                        }
                                        members.append(member);
                                    }
                                    project["members"] = members;

                                    auto resp = drogon::HttpResponse::newHttpJsonResponse(project);
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

void ProjectController::deleteProject(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
) {
    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM delete_project($1, $2)",
        [callback](const drogon::orm::Result& result) {
            Json::Value response;
            response["success"] = true;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            std::string errorMsg = e.base().what();
            Json::Value error;
            if (errorMsg.find("not authorized") != std::string::npos ||
                errorMsg.find("owner") != std::string::npos) {
                error["error"] = "Only the project owner can delete this project";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
            } else {
                error["error"] = "Database error";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
            }
        },
        id,
        userId
    );
}

void ProjectController::inviteMember(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("email")) {
        Json::Value error;
        error["error"] = "Email is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string email = (*json)["email"].asString();
    std::string role = json->isMember("role") ? (*json)["role"].asString() : "member";

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM add_project_member($1, $2, $3)",
        [callback](const drogon::orm::Result& result) {
            Json::Value response;
            response["success"] = true;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            std::string errorMsg = e.base().what();
            Json::Value error;
            if (errorMsg.find("not found") != std::string::npos) {
                error["error"] = "User not found with that email";
            } else if (errorMsg.find("already") != std::string::npos) {
                error["error"] = "User is already a member of this project";
            } else {
                error["error"] = "Database error";
            }
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
        },
        id,
        email,
        role
    );
}

} // namespace controllers
} // namespace kanba
