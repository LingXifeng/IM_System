#pragma once

#include <drogon/WebSocketController.h>
#include "../dao/UserDao.h"
#include "../dao/MessageDao.h"

#include <mutex>
#include <string>
#include <unordered_map>

class ChatController
    : public drogon::WebSocketController<ChatController>
{
public:
    void handleNewMessage(
        const drogon::WebSocketConnectionPtr &wsConn,
        std::string &&message,
        const drogon::WebSocketMessageType &type
    ) override;

    void handleNewConnection(
        const drogon::HttpRequestPtr &req,
        const drogon::WebSocketConnectionPtr &wsConn
    ) override;

    void handleConnectionClosed(
        const drogon::WebSocketConnectionPtr &wsConn
    ) override;

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws");
    WS_PATH_LIST_END

private:
    std::unordered_map<
        std::string,
        drogon::WebSocketConnectionPtr
    > onlineUsers;

    std::mutex mutex;

    UserDao userDao;
    MessageDao messageDao;
};