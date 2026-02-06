#pragma once

#include <drogon/HttpController.h>

namespace kanba {
namespace controllers {

class TaskController : public drogon::HttpController<TaskController> {
public:
    METHOD_LIST_BEGIN
    // All task routes require authentication
    ADD_METHOD_TO(TaskController::createTask, "/api/tasks", drogon::Post, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(TaskController::updateTask, "/api/tasks", drogon::Put, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(TaskController::deleteTask, "/api/tasks", drogon::Delete, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(TaskController::moveTask, "/api/tasks/move", drogon::Post, "kanba::filters::AuthFilter");
    METHOD_LIST_END

    void createTask(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void updateTask(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void deleteTask(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void moveTask(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};

} // namespace controllers
} // namespace kanba
