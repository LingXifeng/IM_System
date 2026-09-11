#include "UserDao.h"

using namespace drogon;
using namespace drogon::orm;

void UserDao::createUser(
    const std::string& username,
    const std::string& passwordHash,
    SuccessCallback success,
    ErrorCallback error)
{
    auto clientPtr = app().getDbClient("im_client");

    if (!clientPtr)
    {
        if (error)
        {
            error("Database client unavailable");
        }
        return;
    }

    clientPtr->execSqlAsync(
        "INSERT INTO users (username, password_hash) "
        "VALUES (?, ?)",

        [success](const Result& result)
        {
            if (success)
            {
                success();
            }
        },

        [error](const DrogonDbException& e)
        {
            if (error)
            {
                std::string message = e.base().what();

                if (message.find("Duplicate entry") != std::string::npos)
                {
                    error("Username already exists");
                }
                else
                {
                    error(
                        std::string("Database error: ") + message
                    );
                }
            }
        },

        username,
        passwordHash
    );
}

void UserDao::findUser(
    const std::string& username,
    LoginSuccessCallback success,
    ErrorCallback error)
{
    auto clientPtr = app().getDbClient("im_client");

    if (!clientPtr)
    {
        if (error)
        {
            error("Database client unavailable");
        }
        return;
    }

    clientPtr->execSqlAsync(
        "SELECT password_hash "
        "FROM users "
        "WHERE username = ? "
        "LIMIT 1",

        [success, error](const Result& result)
        {
            if (result.empty())
            {
                if (error)
                {
                    error("User not found");
                }
                return;
            }

            std::string passwordHash =
                result[0]["password_hash"].as<std::string>();

            if (success)
            {
                success(passwordHash);
            }
        },

        [error](const DrogonDbException& e)
        {
            if (error)
            {
                error(
                    std::string("Database error: ") +
                    e.base().what()
                );
            }
        },

        username
    );
}