#pragma once

#include <drogon/drogon.h>
#include <functional>
#include <string>

class UserDao
{
public:
    using SuccessCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string&)>;

    void createUser(
        const std::string& username,
        const std::string& passwordHash,
        SuccessCallback success,
        ErrorCallback error);
};