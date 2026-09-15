#pragma once

#include <drogon/drogon.h>
#include <functional>
#include <string>
#include <vector>

class UserDao
{
public:
    using SuccessCallback =
        std::function<void()>;

    using ErrorCallback =
        std::function<void(const std::string&)>;

    void createUser(
        const std::string& username,
        const std::string& passwordHash,
        SuccessCallback success,
        ErrorCallback error
    );

    using LoginSuccessCallback =
        std::function<void(const std::string& passwordHash)>;

    void findUser(
        const std::string& username,
        LoginSuccessCallback success,
        ErrorCallback error
    );

    using UserIdSuccessCallback =
        std::function<void(long long userId)>;

    void findUserId(
        const std::string& username,
        UserIdSuccessCallback success,
        ErrorCallback error
    );
    using UsernameSuccessCallback =
    std::function<void(const std::string& username)>;

    void findUsernameById(
        long long userId,
        UsernameSuccessCallback success,
        ErrorCallback error
    );
    using SearchUserCallback =
    std::function<void(const std::vector<std::string>&)>;

    void searchUsers(
        const std::string& keyword,
        SearchUserCallback success,
        ErrorCallback error
    );
};