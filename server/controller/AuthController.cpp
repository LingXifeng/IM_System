#include "AuthController.h"
#include "../service/AuthService.h"

using namespace drogon;

void AuthController::registerUser(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    // 防止未使用参数警告
    (void)req;

    
    // 1获取 JSON


    auto jsonPtr = req->getJsonObject();

    if (!jsonPtr)
    {
        auto resp = HttpResponse::newHttpResponse();

        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");

        callback(resp);
        return;
    }

    
    // 检查参数
    

    if (!jsonPtr->isMember("username") ||
        !jsonPtr->isMember("password"))
    {
        auto resp = HttpResponse::newHttpResponse();

        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing username or password");

        callback(resp);
        return;
    }

    std::string username =
        (*jsonPtr)["username"].asString();

    std::string password =
        (*jsonPtr)["password"].asString();

    
    // 保存 HTTP callback
    

    auto callbackPtr =
        std::make_shared<
            std::function<void(const HttpResponsePtr&)>
        >(std::move(callback));


    // 调用 Service


    auto service =
        std::make_shared<AuthService>();

    service->registerUser(
        username,
        password,

        // 成功回调
        [callbackPtr]()
        {
            Json::Value respJson;

            respJson["status"] = "success";
            respJson["message"] =
                "User registered successfully";

            auto resp =
                HttpResponse::newHttpJsonResponse(
                    respJson
                );

            (*callbackPtr)(resp);
        },

        // 失败回调
        [callbackPtr](const std::string& errorMessage)
        {
            Json::Value respJson;
            respJson["status"] = "error";
            respJson["message"] = errorMessage;

            auto resp =
                HttpResponse::newHttpJsonResponse(respJson);

            if (errorMessage == "Username already exists")
            {
                resp->setStatusCode(k409Conflict);
            }
            else
            {
                resp->setStatusCode(k500InternalServerError);
            }

            (*callbackPtr)(resp);
        }
    );
}

void AuthController::loginUser(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto jsonPtr = req->getJsonObject();

    if (!jsonPtr)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    if (!jsonPtr->isMember("username") ||
        !jsonPtr->isMember("password"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing username or password");
        callback(resp);
        return;
    }

    std::string username =
        (*jsonPtr)["username"].asString();

    std::string password =
        (*jsonPtr)["password"].asString();

    auto callbackPtr =
        std::make_shared<
            std::function<void(const HttpResponsePtr&)>
        >(std::move(callback));

    auto service =
        std::make_shared<AuthService>();

    service->loginUser(
        username,
        password,

        [callbackPtr]()
        {
            Json::Value respJson;
            respJson["status"] = "success";
            respJson["message"] = "Login successful";

            auto resp =
                HttpResponse::newHttpJsonResponse(respJson);

            (*callbackPtr)(resp);
        },

        [callbackPtr](
            const std::string& errorMessage)
        {
            Json::Value respJson;
            respJson["status"] = "error";
            respJson["message"] = errorMessage;

            auto resp =
                HttpResponse::newHttpJsonResponse(respJson);

            if (errorMessage == "User not found" ||
                errorMessage == "Invalid username or password")
            {
                resp->setStatusCode(k401Unauthorized);
            }
            else
            {
                resp->setStatusCode(
                    k500InternalServerError);
            }

            (*callbackPtr)(resp);
        }
    );
}