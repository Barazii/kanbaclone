#pragma once

#include <drogon/HttpController.h>

namespace kanba {
namespace controllers {

class AiChatController : public drogon::HttpController<AiChatController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AiChatController::chat, "/api/ai-chat", drogon::Post);
    METHOD_LIST_END

    void chat(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};

} // namespace controllers
} // namespace kanba
