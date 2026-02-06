#include "AiChatController.h"
#include <drogon/HttpClient.h>

namespace kanba {
namespace controllers {

void AiChatController::chat(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("messages") || !json->isMember("apiKey")) {
        Json::Value error;
        error["error"] = "Messages and API key are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string apiKey = (*json)["apiKey"].asString();
    Json::Value messages = (*json)["messages"];

    // Build OpenAI request
    Json::Value openaiRequest;
    openaiRequest["model"] = "gpt-4o-mini";
    openaiRequest["messages"] = messages;
    openaiRequest["max_tokens"] = 1000;

    Json::StreamWriterBuilder writer;
    std::string requestBody = Json::writeString(writer, openaiRequest);

    // Create HTTP client for OpenAI
    auto client = drogon::HttpClient::newHttpClient("https://api.openai.com");

    auto openaiReq = drogon::HttpRequest::newHttpRequest();
    openaiReq->setPath("/v1/chat/completions");
    openaiReq->setMethod(drogon::Post);
    openaiReq->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    openaiReq->setBody(requestBody);
    openaiReq->addHeader("Authorization", "Bearer " + apiKey);

    client->sendRequest(
        openaiReq,
        [callback](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            if (result != drogon::ReqResult::Ok) {
                Json::Value error;
                error["error"] = "Failed to connect to OpenAI API";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k502BadGateway);
                callback(resp);
                return;
            }

            auto responseJson = response->getJsonObject();
            if (!responseJson) {
                Json::Value error;
                error["error"] = "Invalid response from OpenAI API";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k502BadGateway);
                callback(resp);
                return;
            }

            if (response->statusCode() != drogon::k200OK) {
                Json::Value error;
                if (responseJson->isMember("error")) {
                    error["error"] = (*responseJson)["error"]["message"].asString();
                } else {
                    error["error"] = "OpenAI API error";
                }
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(response->statusCode());
                callback(resp);
                return;
            }

            // Extract the message content
            std::string message;
            if (responseJson->isMember("choices") &&
                (*responseJson)["choices"].isArray() &&
                (*responseJson)["choices"].size() > 0 &&
                (*responseJson)["choices"][0].isMember("message") &&
                (*responseJson)["choices"][0]["message"].isMember("content")) {
                message = (*responseJson)["choices"][0]["message"]["content"].asString();
            }

            Json::Value jsonResp;
            jsonResp["message"] = message;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResp);
            callback(resp);
        },
        30.0  // 30 second timeout
    );
}

} // namespace controllers
} // namespace kanba
