#include "FriendDao.h"

#include <drogon/drogon.h>

using namespace drogon;
using namespace drogon::orm;

void FriendDao::addFriend(
    long long userId,
    long long friendId,
    SuccessCallback success,
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
        "INSERT INTO friends "
        "(user_id, friend_id) "
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
                std::string message =
                    e.base().what();

                if (message.find("Duplicate entry")
                    != std::string::npos)
                {
                    error("Already friends");
                }
                else
                {
                    error(
                        std::string("Database error: ")
                        + message
                    );
                }
            }
        },

        userId,
        friendId
    );
}


void FriendDao::deleteFriend(
    long long userId,
    long long friendId,
    SuccessCallback success,
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
        "DELETE FROM friends "
        "WHERE user_id = ? "
        "AND friend_id = ?",

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
                error(
                    std::string("Database error: ")
                    + e.base().what()
                );
            }
        },

        userId,
        friendId
    );
}


void FriendDao::getFriendList(
    long long userId,
    FriendListCallback success,
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
        "SELECT f.id, f.user_id, f.friend_id, "
        "u.username "
        "FROM friends f "
        "JOIN users u "
        "ON f.friend_id = u.id "
        "WHERE f.user_id = ? "
        "ORDER BY f.id ASC",

        [success](const Result& result)
        {
            std::vector<Friend> friends;

            for (const auto& row : result)
            {
                Friend friendItem;

                friendItem.id =
                    row["id"].as<long long>();

                friendItem.userId =
                    row["user_id"].as<long long>();

                friendItem.friendId =
                    row["friend_id"].as<long long>();

                friendItem.username =
                    row["username"].as<std::string>();

                friends.push_back(friendItem);
            }

            if (success)
            {
                success(friends);
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


void FriendDao::isFriend(
    long long userId,
    long long friendId,
    IsFriendCallback success,
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
        "FROM friends "
        "WHERE user_id = ? "
        "AND friend_id = ? "
        "LIMIT 1",

        [success](const Result& result)
        {
            if (success)
            {
                success(!result.empty());
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

        userId,
        friendId
    );
}