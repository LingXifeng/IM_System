#include "FriendController.h"

#include <drogon/drogon.h>
#include <json/json.h>

using namespace drogon;

FriendDao FriendController::friendDao;
UserDao FriendController::userDao;


void FriendController::addFriend(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    if (!json)
    {
        auto resp =
            HttpResponse::newHttpJsonResponse(
                Json::Value(
                    Json::objectValue
                )
            );

        resp->setStatusCode(
            k400BadRequest
        );

        (*resp->getJsonObject())["message"] =
            "Invalid JSON";

        callback(resp);
        return;
    }

    if (!json->isMember("username") ||
        !json->isMember("friendUsername"))
    {
        Json::Value body;

        body["message"] =
            "username and friendUsername are required";

        auto resp =
            HttpResponse::newHttpJsonResponse(body);

        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    std::string username =
        (*json)["username"].asString();

    std::string friendUsername =
        (*json)["friendUsername"].asString();

    if (username.empty() ||
        friendUsername.empty())
    {
        Json::Value body;

        body["message"] =
            "Username cannot be empty";

        auto resp =
            HttpResponse::newHttpJsonResponse(body);

        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    if (username == friendUsername)
    {
        Json::Value body;

        body["message"] =
            "Cannot add yourself";

        auto resp =
            HttpResponse::newHttpJsonResponse(body);

        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    // 先查询当前用户 ID
    userDao.findUserId(
        username,

        [friendUsername, callback]
        (long long userId)
        {
            // 再查询好友 ID
            FriendController::userDao.findUserId(
                friendUsername,

                [userId, friendUsername, callback]
                (long long friendId)
                {
                    // 创建 user -> friend
                    FriendController::friendDao.addFriend(
                        userId,
                        friendId,

                        [userId, friendId, friendUsername, callback]()
                        {
                            // 创建 friend -> user
                            FriendController::friendDao.addFriend(
                                friendId,
                                userId,

                                [friendUsername, callback]()
                                {
                                    Json::Value body;

                                    body["message"] =
                                        "Friend added successfully";

                                    body["friendUsername"] =
                                        friendUsername;

                                    auto resp =
                                        HttpResponse::
                                        newHttpJsonResponse(body);

                                    callback(resp);
                                },

                                [callback](const std::string& error)
                                {
                                    Json::Value body;

                                    body["message"] =
                                        error;

                                    auto resp =
                                        HttpResponse::
                                        newHttpJsonResponse(body);

                                    resp->setStatusCode(
                                        k500InternalServerError
                                    );

                                    callback(resp);
                                }
                            );
                        },

                        [callback](const std::string& error)
                        {
                            Json::Value body;

                            body["message"] =
                                error;

                            auto resp =
                                HttpResponse::
                                newHttpJsonResponse(body);

                            resp->setStatusCode(
                                k400BadRequest
                            );

                            callback(resp);
                        }
                    );
                },

                [callback](const std::string& error)
                {
                    Json::Value body;

                    body["message"] =
                        error;

                    auto resp =
                        HttpResponse::
                        newHttpJsonResponse(body);

                    resp->setStatusCode(
                        k404NotFound
                    );

                    callback(resp);
                }
            );
        },

        [callback](const std::string& error)
        {
            Json::Value body;

            body["message"] =
                error;

            auto resp =
                HttpResponse::
                newHttpJsonResponse(body);

            resp->setStatusCode(
                k404NotFound
            );

            callback(resp);
        }
    );
}


void FriendController::deleteFriend(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    if (!json)
    {
        Json::Value body;

        body["message"] =
            "Invalid JSON";

        auto resp =
            HttpResponse::newHttpJsonResponse(body);

        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    if (!json->isMember("username") ||
        !json->isMember("friendUsername"))
    {
        Json::Value body;

        body["message"] =
            "username and friendUsername are required";

        auto resp =
            HttpResponse::newHttpJsonResponse(body);

        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    std::string username =
        (*json)["username"].asString();

    std::string friendUsername =
        (*json)["friendUsername"].asString();

    userDao.findUserId(
        username,

        [friendUsername, callback]
        (long long userId)
        {
            FriendController::userDao.findUserId(
                friendUsername,

                [userId, callback]
                (long long friendId)
                {
                    // 删除 user -> friend
                    FriendController::friendDao.deleteFriend(
                        userId,
                        friendId,

                        [friendId, userId, callback]()
                        {
                            // 删除 friend -> user
                            FriendController::friendDao.deleteFriend(
                                friendId,
                                userId,

                                [callback]()
                                {
                                    Json::Value body;

                                    body["message"] =
                                        "Friend deleted successfully";

                                    auto resp =
                                        HttpResponse::
                                        newHttpJsonResponse(body);

                                    callback(resp);
                                },

                                [callback](const std::string& error)
                                {
                                    Json::Value body;

                                    body["message"] =
                                        error;

                                    auto resp =
                                        HttpResponse::
                                        newHttpJsonResponse(body);

                                    resp->setStatusCode(
                                        k500InternalServerError
                                    );

                                    callback(resp);
                                }
                            );
                        },

                        [callback](const std::string& error)
                        {
                            Json::Value body;

                            body["message"] =
                                error;

                            auto resp =
                                HttpResponse::
                                newHttpJsonResponse(body);

                            resp->setStatusCode(
                                k500InternalServerError
                            );

                            callback(resp);
                        }
                    );
                },

                [callback](const std::string& error)
                {
                    Json::Value body;

                    body["message"] =
                        error;

                    auto resp =
                        HttpResponse::
                        newHttpJsonResponse(body);

                    resp->setStatusCode(
                        k404NotFound
                    );

                    callback(resp);
                }
            );
        },

        [callback](const std::string& error)
        {
            Json::Value body;

            body["message"] =
                error;

            auto resp =
                HttpResponse::
                newHttpJsonResponse(body);

            resp->setStatusCode(
                k404NotFound
            );

            callback(resp);
        }
    );
}


void FriendController::getFriendList(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    std::string username =
        req->getParameter("username");

    if (username.empty())
    {
        Json::Value body;

        body["message"] =
            "username is required";

        auto resp =
            HttpResponse::newHttpJsonResponse(body);

        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    userDao.findUserId(
        username,

        [callback](long long userId)
        {
            FriendController::friendDao.getFriendList(
                userId,

                [callback](const std::vector<Friend>& friends)
                {
                    Json::Value array(
                        Json::arrayValue
                    );

                    for (const auto& item : friends)
                    {
                        Json::Value friendJson;

                        friendJson["id"] =
                            static_cast<Json::Int64>(
                                item.id
                            );

                        friendJson["friendId"] =
                        static_cast<Json::Int64>(
                            item.friendId
                            );

                        friendJson["username"] =
                            item.username;

                        array.append(friendJson);
                    }

                    Json::Value body;

                    body["friends"] = array;

                    auto resp =
                        HttpResponse::
                        newHttpJsonResponse(body);

                    callback(resp);
                },

                [callback](const std::string& error)
                {
                    Json::Value body;

                    body["message"] =
                        error;

                    auto resp =
                        HttpResponse::
                        newHttpJsonResponse(body);

                    resp->setStatusCode(
                        k500InternalServerError
                    );

                    callback(resp);
                }
            );
        },

        [callback](const std::string& error)
        {
            Json::Value body;

            body["message"] =
                error;

            auto resp =
                HttpResponse::newHttpJsonResponse(body);

            resp->setStatusCode(
                k404NotFound
            );

            callback(resp);
        }
    );
}