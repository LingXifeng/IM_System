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