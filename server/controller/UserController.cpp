#include "UserController.h"

UserDao UserController::userDao;

void UserController::searchUsers(
    const drogon::HttpRequestPtr& req,
    std::function<void(
        const drogon::HttpResponsePtr&)>&& callback)
{
    std::string keyword =
        req->getParameter("username");

    if (keyword.empty())
    {
        Json::Value root;
        root["message"] = "搜索关键词不能为空";

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(root);

        resp->setStatusCode(
            drogon::k400BadRequest
        );

        callback(resp);
        return;
    }

    userDao.searchUsers(
        keyword,

        [callback](
            const std::vector<std::string>& users)
        {
            Json::Value root;
            root["users"] =
                Json::Value(Json::arrayValue);

            for (const auto& username : users)
            {
                root["users"].append(username);
            }

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(root);

            callback(resp);
        },

        [callback](const std::string& error)
        {
            Json::Value root;
            root["message"] = error;

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(root);

            resp->setStatusCode(
                drogon::k500InternalServerError
            );

            callback(resp);
        }
    );
}