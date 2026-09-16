#include "MessageDao.h"

#include <drogon/drogon.h>

using namespace drogon;
using namespace drogon::orm;


void MessageDao::saveMessage(
    long long senderId,
    long long receiverId,
    const std::string& content,
    long long ttlSeconds,
    SaveSuccessCallback success,
    ErrorCallback error)
{
    auto dbClient =
        drogon::app().getDbClient("im_client");

    if (!dbClient)
    {
        if (error)
        {
            error("Database client unavailable");
        }

        return;
    }

    /*
     * ttlSeconds == 0:
     *     普通消息，expire_at 为 NULL
     *
     * ttlSeconds > 0:
     *     expire_at = 当前数据库时间 + TTL 秒
     *
     * 使用 NULLIF(?, 0) 判断是否需要设置过期时间。
     */
    dbClient->execSqlAsync(
        "INSERT INTO messages "
        "(sender_id, receiver_id, content, expire_at) "
        "VALUES (?, ?, ?, "
        "CASE "
        "WHEN ? > 0 THEN DATE_ADD(NOW(), INTERVAL ? SECOND) "
        "ELSE NULL "
        "END)",

        [success](const drogon::orm::Result& result)
        {
            long long messageId =
                result.insertId();

            if (success)
            {
                success(messageId);
            }
        },

        [error](const drogon::orm::DrogonDbException& e)
        {
            if (error)
            {
                error(e.base().what());
            }
        },

        senderId,
        receiverId,
        content,
        ttlSeconds,
        ttlSeconds
    );
}


void MessageDao::getOfflineMessages(
    long long receiverId,
    OfflineMessageCallback success,
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
        "SELECT id, sender_id, receiver_id, "
        "content, status, created_at, expire_at "
        "FROM messages "
        "WHERE receiver_id = ? "
        "AND status = 0 "
        "AND (expire_at IS NULL OR expire_at > NOW()) "
        "ORDER BY id ASC",

        [success, error](const Result& result)
        {
            std::vector<Message> messages;

            for (const auto& row : result)
            {
                Message message;

                message.id =
                    row["id"].as<long long>();

                message.senderId =
                    row["sender_id"].as<long long>();

                message.receiverId =
                    row["receiver_id"].as<long long>();

                message.content =
                    row["content"].as<std::string>();

                message.status =
                    row["status"].as<int>();

                message.createdAt =
                    row["created_at"].as<std::string>();

                if (!row["expire_at"].isNull())
                {
                    message.expireAt =
                        row["expire_at"].as<std::string>();
                }

                messages.push_back(message);
            }

            if (success)
            {
                success(messages);
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

        receiverId
    );
}


void MessageDao::getChatHistory(
    long long userId,
    long long friendId,
    OfflineMessageCallback success,
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
        "SELECT id, sender_id, receiver_id, "
        "content, status, created_at, expire_at "
        "FROM messages "
        "WHERE "
        "((sender_id = ? AND receiver_id = ?) "
        "OR "
        "(sender_id = ? AND receiver_id = ?)) "
        "AND "
        "(expire_at IS NULL OR expire_at > NOW()) "
        "ORDER BY id ASC",

        [success](
            const Result& result)
        {
            std::vector<Message> messages;

            for (const auto& row : result)
            {
                Message message;

                message.id =
                    row["id"].as<long long>();

                message.senderId =
                    row["sender_id"].as<long long>();

                message.receiverId =
                    row["receiver_id"].as<long long>();

                message.content =
                    row["content"].as<std::string>();

                message.status =
                    row["status"].as<int>();

                message.createdAt =
                    row["created_at"].as<std::string>();

                if (!row["expire_at"].isNull())
                {
                    message.expireAt =
                        row["expire_at"].as<std::string>();
                }

                messages.push_back(message);
            }

            if (success)
            {
                success(messages);
            }
        },

        [error](
            const DrogonDbException& e)
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
        friendId,
        friendId,
        userId
    );
}


void MessageDao::markAsDelivered(
    long long messageId,
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
        "UPDATE messages "
        "SET status = 1 "
        "WHERE id = ?",

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

        messageId
    );
}


void MessageDao::markAsRead(
    long long messageId,
    SuccessCallback success,
    ErrorCallback error)
{
    auto dbClient =
        drogon::app().getDbClient("im_client");

    if (!dbClient)
    {
        if (error)
        {
            error("Database client unavailable");
        }

        return;
    }

    dbClient->execSqlAsync(
        "UPDATE messages "
        "SET status = 2 "
        "WHERE id = ?",

        [success](const drogon::orm::Result&)
        {
            if (success)
            {
                success();
            }
        },

        [error](const drogon::orm::DrogonDbException& e)
        {
            if (error)
            {
                error(e.base().what());
            }
        },

        messageId
    );
}


void MessageDao::getMessageById(
    long long messageId,
    MessageCallback success,
    ErrorCallback error)
{
    auto dbClient =
        drogon::app().getDbClient("im_client");

    if (!dbClient)
    {
        if (error)
        {
            error("Database client unavailable");
        }

        return;
    }

    dbClient->execSqlAsync(
        "SELECT id, sender_id, receiver_id, "
        "content, status, created_at, expire_at "
        "FROM messages "
        "WHERE id = ?",

        [success, error](
            const drogon::orm::Result& result)
        {
            if (result.empty())
            {
                if (error)
                {
                    error("Message not found");
                }

                return;
            }

            const auto& row = result[0];

            Message message;

            message.id =
                row["id"].as<long long>();

            message.senderId =
                row["sender_id"].as<long long>();

            message.receiverId =
                row["receiver_id"].as<long long>();

            message.content =
                row["content"].as<std::string>();

            message.status =
                row["status"].as<int>();

            message.createdAt =
                row["created_at"].as<std::string>();

            if (!row["expire_at"].isNull())
            {
                message.expireAt =
                    row["expire_at"].as<std::string>();
            }

            if (success)
            {
                success(message);
            }
        },

        [error](
            const drogon::orm::DrogonDbException& e)
        {
            if (error)
            {
                error(e.base().what());
            }
        },

        messageId
    );
}