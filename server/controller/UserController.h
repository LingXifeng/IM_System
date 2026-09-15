#pragma once

#include "../dao/UserDao.h"
#include <drogon/drogon.h>

class UserController
{
public:
    static void searchUsers(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&)>&& callback
    );

private:
    static UserDao userDao;
};