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

void UserDao::findUserId(
    const std::string& username,
    UserIdSuccessCallback success,
    ErrorCallback error)
{
    auto clientPtr =
        app().getDbClient("im_client");

    if (!clientPtr)
    {
        if (error)
        {
            error("Database client unavailable");
        }

        return;
    }

    clientPtr->execSqlAsync(
        "SELECT id "
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

            long long userId =
                result[0]["id"].as<long long>();

            if (success)
            {
                success(userId);
            }
        },

        [error](const DrogonDbException& e)
        {
            if (error)
            {
                error(
                    std::string("Database error: ")
                    + e.base().what()
                );
            }
        },

        username
    );
}

 void UserDao::findUsernameById(
    long long userId,
    UsernameSuccessCallback success,
    ErrorCallback error)
{
    auto clientPtr =
        app().getDbClient("im_client");

    if (!clientPtr)
    {
        if (error)
        {
            error("Database client unavailable");
        }

        return;
    }

    clientPtr->execSqlAsync(
        "SELECT username "
        "FROM users "
        "WHERE id = ? "
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

            std::string username =
                result[0]["username"].as<std::string>();

            if (success)
            {
                success(username);
            }
        },

        [error](const DrogonDbException& e)
        {
            if (error)
            {
                error(
                    std::string("Database error: ")
                    + e.base().what()
                );
            }
        },

        userId
    );
}

void UserDao::searchUsers(
    const std::string& keyword,
    SearchUserCallback success,
    ErrorCallback error)
{
    auto dbClient =
    drogon::app().getDbClient("im_client");

    dbClient->execSqlAsync(
        "SELECT username "
        "FROM users "
        "WHERE username LIKE ? "
        "LIMIT 20",

        [success](const drogon::orm::Result &result)
        {
            std::vector<std::string> users;

            for (const auto &row : result)
            {
                users.push_back(
                    row["username"].as<std::string>()
                );
            }

            success(users);
        },

        [error](const drogon::orm::DrogonDbException &e)
        {
            error(e.base().what());
        },

        "%" + keyword + "%"
    );
}