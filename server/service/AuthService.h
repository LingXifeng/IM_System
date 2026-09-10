#pragma once

#include <drogon/drogon.h>
#include <functional>
#include <string>

class AuthService
{
public:
    using SuccessCallback = std::function<void()>;
    using ErrorCallback =
        std::function<void(const std::string&)>;

    void registerUser(
        const std::string& username,
        const std::string& password,
        SuccessCallback success,
        ErrorCallback error);
};