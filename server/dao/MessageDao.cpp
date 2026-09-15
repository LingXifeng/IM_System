#include "MessageDao.h"

#include <drogon/drogon.h>

using namespace drogon;
using namespace drogon::orm;

void MessageDao::saveMessage(
    long long senderId,
    long long receiverId,
    const std::string& content,
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
        "INSERT INTO messages "
        "(sender_id, receiver_id, content) "
        "VALUES (?, ?, ?)",

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

        senderId,
        receiverId,
        content
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
        "content, status, created_at "
        "FROM messages "
        "WHERE receiver_id = ? "
        "AND status = 0 "
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