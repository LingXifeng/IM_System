#include "AuthService.h"
#include "../dao/UserDao.h"

void AuthService::registerUser(
    const std::string& username,
    const std::string& password,
    SuccessCallback success,
    ErrorCallback error)
{
    // 基础业务校验

    if (username.empty())
    {
        if (error)
        {
            error("Username cannot be empty");
        }
        return;
    }

    if (password.empty())
    {
        if (error)
        {
            error("Password cannot be empty");
        }
        return;
    }

    // 使用密码 
    

    std::string passwordHash = password;

    // ===== 调用 DAO =====

    auto dao = std::make_shared<UserDao>();

    dao->createUser(
        username,
        passwordHash,

        [success]()
        {
            if (success)
            {
                success();
            }
        },

        [error](const std::string& errorMessage)
        {
            if (error)
            {
                error(errorMessage);
            }
        }
    );
}

void AuthService::loginUser(
    const std::string& username,
    const std::string& password,
    LoginSuccessCallback success,
    ErrorCallback error)
{
    if (username.empty())
    {
        if (error)
        {
            error("Username cannot be empty");
        }
        return;
    }

    if (password.empty())
    {
        if (error)
        {
            error("Password cannot be empty");
        }
        return;
    }

    auto dao = std::make_shared<UserDao>();

    dao->findUser(
        username,

        [password, success, error](
            const std::string& passwordHash)
        {
            // V1 暂时使用明文密码
            if (password != passwordHash)
            {
                if (error)
                {
                    error("Invalid username or password");
                }
                return;
            }

            if (success)
            {
                success();
            }
        },

        [error](const std::string& errorMessage)
        {
            if (error)
            {
                error(errorMessage);
            }
        }
    );
}