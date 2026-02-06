#include "AuthController.h"
#include "../utils/Database.h"
#include "../utils/Session.h"
#include "../utils/PasswordHash.h"
#include "../filters/AuthFilter.h"
#include <cstdlib>

namespace kanba {
namespace controllers {

void AuthController::setSessionCookie(
    drogon::HttpResponsePtr& resp,
    const std::string& sessionId,
    bool clear
) const {
    drogon::Cookie cookie(utils::Session::COOKIE_NAME, sessionId);
    cookie.setHttpOnly(true);
    cookie.setPath("/");

    const char* nodeEnv = std::getenv("NODE_ENV");
    bool isProduction = nodeEnv && std::string(nodeEnv) == "production";

    if (isProduction) {
        cookie.setSecure(true);
        cookie.setSameSite(drogon::Cookie::SameSite::kNone);
    } else {
        cookie.setSameSite(drogon::Cookie::SameSite::kLax);
    }

    if (clear) {
        cookie.setMaxAge(0);
    } else {
        cookie.setMaxAge(utils::Session::SESSION_TTL_SECONDS);
    }

    resp->addCookie(cookie);
}

void AuthController::login(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("email") || !json->isMember("password")) {
        Json::Value error;
        error["error"] = "Email and password are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM get_user_by_email($1)",
        [this, password, callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value error;
                error["error"] = "Invalid email or password";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k401Unauthorized);
                callback(resp);
                return;
            }

            auto row = result[0];
            std::string storedHash = row["password_hash"].as<std::string>();
            std::string userId = row["id"].as<std::string>();
            std::string name = row["name"].as<std::string>();
            std::string userEmail = row["email"].as<std::string>();

            // Verify password
            if (!utils::PasswordHash::verify(password, storedHash)) {
                Json::Value error;
                error["error"] = "Invalid email or password";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k401Unauthorized);
                callback(resp);
                return;
            }

            // Create session
            std::string sessionId = utils::Session::generateSessionId();
            utils::Session::createSession(
                sessionId,
                userId,
                [this, sessionId, userId, name, userEmail, callback](bool success) {
                    if (!success) {
                        Json::Value error;
                        error["error"] = "Failed to create session";
                        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                        resp->setStatusCode(drogon::k500InternalServerError);
                        callback(resp);
                        return;
                    }

                    Json::Value user;
                    user["id"] = userId;
                    user["name"] = name;
                    user["email"] = userEmail;

                    Json::Value response;
                    response["user"] = user;

                    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                    setSessionCookie(resp, sessionId);
                    callback(resp);
                }
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value error;
            error["error"] = "Database error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        email
    );
}

void AuthController::registerUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("email") || !json->isMember("password") || !json->isMember("name")) {
        Json::Value error;
        error["error"] = "Email, password, and name are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    std::string name = (*json)["name"].asString();

    // Hash password
    std::string passwordHash;
    try {
        passwordHash = utils::PasswordHash::hash(password);
    } catch (const std::exception& e) {
        Json::Value error;
        error["error"] = "Failed to hash password";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "SELECT * FROM create_user($1, $2, $3)",
        [this, callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value error;
                error["error"] = "Failed to create user";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
                return;
            }

            auto row = result[0];
            std::string userId = row["id"].as<std::string>();
            std::string userName = row["name"].as<std::string>();
            std::string userEmail = row["email"].as<std::string>();

            // Create session
            std::string sessionId = utils::Session::generateSessionId();
            utils::Session::createSession(
                sessionId,
                userId,
                [this, sessionId, userId, userName, userEmail, callback](bool success) {
                    if (!success) {
                        Json::Value error;
                        error["error"] = "Failed to create session";
                        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                        resp->setStatusCode(drogon::k500InternalServerError);
                        callback(resp);
                        return;
                    }

                    Json::Value user;
                    user["id"] = userId;
                    user["name"] = userName;
                    user["email"] = userEmail;

                    Json::Value response;
                    response["user"] = user;

                    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                    setSessionCookie(resp, sessionId);
                    callback(resp);
                }
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value error;
            if (std::string(e.base().what()).find("duplicate") != std::string::npos) {
                error["error"] = "Email already registered";
            } else {
                error["error"] = "Database error";
            }
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
        },
        email,
        passwordHash,
        name
    );
}

void AuthController::logout(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    std::string sessionId = req->getCookie(utils::Session::COOKIE_NAME);

    if (!sessionId.empty()) {
        utils::Session::deleteSession(sessionId, [](bool) {});
    }

    Json::Value response;
    response["success"] = true;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    setSessionCookie(resp, "", true);
    callback(resp);
}

void AuthController::me(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    std::string sessionId = req->getCookie(utils::Session::COOKIE_NAME);

    if (sessionId.empty()) {
        Json::Value response;
        response["user"] = Json::nullValue;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);
        return;
    }

    utils::Session::getUserIdFromSession(
        sessionId,
        [callback](std::optional<std::string> userId) {
            if (!userId.has_value()) {
                Json::Value response;
                response["user"] = Json::nullValue;
                auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                callback(resp);
                return;
            }

            auto db = utils::Database::getClient();
            db->execSqlAsync(
                "SELECT * FROM get_user_by_id($1)",
                [callback](const drogon::orm::Result& result) {
                    Json::Value response;
                    if (result.empty()) {
                        response["user"] = Json::nullValue;
                    } else {
                        auto row = result[0];
                        Json::Value user;
                        user["id"] = row["id"].as<std::string>();
                        user["name"] = row["name"].as<std::string>();
                        user["email"] = row["email"].as<std::string>();
                        if (!row["avatar_url"].isNull()) {
                            user["avatar_url"] = row["avatar_url"].as<std::string>();
                        }
                        response["user"] = user;
                    }
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    Json::Value response;
                    response["user"] = Json::nullValue;
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                    callback(resp);
                },
                *userId
            );
        }
    );
}

void AuthController::update(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("name")) {
        Json::Value error;
        error["error"] = "Name is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string userId = req->attributes()->get<std::string>(filters::AuthFilter::USER_ID_KEY);
    std::string name = (*json)["name"].asString();

    auto db = utils::Database::getClient();
    db->execSqlAsync(
        "UPDATE users SET name = $1 WHERE id = $2 RETURNING id, email, name, avatar_url",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value error;
                error["error"] = "User not found";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            auto row = result[0];
            Json::Value user;
            user["id"] = row["id"].as<std::string>();
            user["name"] = row["name"].as<std::string>();
            user["email"] = row["email"].as<std::string>();
            if (!row["avatar_url"].isNull()) {
                user["avatar_url"] = row["avatar_url"].as<std::string>();
            }

            Json::Value response;
            response["user"] = user;

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
        name,
        userId
    );
}

} // namespace controllers
} // namespace kanba
