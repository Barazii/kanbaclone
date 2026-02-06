#pragma once

#include <drogon/HttpController.h>

namespace kanba {
namespace controllers {

class ColumnController : public drogon::HttpController<ColumnController> {
public:
    METHOD_LIST_BEGIN
    // All column routes require authentication
    ADD_METHOD_TO(ColumnController::createColumn, "/api/columns", drogon::Post, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(ColumnController::updateColumn, "/api/columns", drogon::Put, "kanba::filters::AuthFilter");
    ADD_METHOD_TO(ColumnController::deleteColumn, "/api/columns", drogon::Delete, "kanba::filters::AuthFilter");
    METHOD_LIST_END

    void createColumn(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void updateColumn(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void deleteColumn(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};

} // namespace controllers
} // namespace kanba
