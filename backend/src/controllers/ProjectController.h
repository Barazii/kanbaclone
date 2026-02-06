#pragma once

#include <drogon/HttpController.h>

namespace kanba {
namespace controllers {

class ProjectController : public drogon::HttpController<ProjectController> {
public:
    METHOD_LIST_BEGIN
    // All project routes require authentication
    ADD_METHOD_TO(ProjectController::getProjects, "/api/projects", drogon::Get, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(ProjectController::createProject, "/api/projects", drogon::Post, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(ProjectController::getProject, "/api/projects/{id}", drogon::Get, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(ProjectController::deleteProject, "/api/projects/{id}", drogon::Delete, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(ProjectController::inviteMember, "/api/projects/{id}/invite", drogon::Post, "kanba::filters::AuthFilter");
    METHOD_LIST_END

    void getProjects(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void createProject(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void getProject(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
    );

    void deleteProject(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
    );

    void inviteMember(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
    );
};

} // namespace controllers
} // namespace kanba
