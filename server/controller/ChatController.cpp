#include "ChatController.h"

#include <iostream>
#include <sstream>
#include <json/json.h>

using namespace drogon;

void ChatController::handleNewConnection(
    const HttpRequestPtr &req,
    const WebSocketConnectionPtr &wsConn)
{
    std::string username =
        req->getParameter("username");

    if (username.empty())
    {
        std::cout
            << "[WebSocket] Connection rejected: "
            << "username is empty"
            << std::endl;

        wsConn->shutdown();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);

        onlineUsers[username] = wsConn;
    }

    std::cout
        << "[WebSocket] User connected: "
        << username
        << std::endl;

    wsConn->send(
        "WebSocket connected as " + username
    );

    // 1. 根据用户名查找用户 ID
    userDao.findUserId(
        username,

        [this, wsConn, username](long long userId)
        {
            // 2. 查询该用户的离线消息
            messageDao.getOfflineMessages(
                userId,

                [this, wsConn, username]
                (const std::vector<Message>& messages)
                {
                    // 没有离线消息
                    if (messages.empty())
                    {
                        std::cout
                            << "[WebSocket] No offline messages for "
                            << username
                            << std::endl;

                        return;
                    }

                    for (const auto& message : messages)
                    {
                        // 3. 根据 senderId 查找发送者用户名
                        userDao.findUsernameById(
                            message.senderId,

                            [this, wsConn, message]
                            (const std::string& senderUsername)
                            {
                                Json::Value response;

                                response["type"] =
                                    "message";

                                response["from"] =
                                    senderUsername;

                                response["content"] =
                                    message.content;

                                response["messageId"] =
                                    static_cast<Json::Int64>(
                                        message.id
                                    );

                                Json::StreamWriterBuilder
                                    writer;

                                // 4. 推送给重新上线的用户
                                wsConn->send(
                                    Json::writeString(
                                        writer,
                                        response
                                    )
                                );

                                // 5. 标记为已投递
                                messageDao.markAsDelivered(
                                    message.id,

                                    [message]()
                                    {
                                        std::cout
                                            << "[MessageDao] "
                                               "Offline message "
                                               "delivered: "
                                            << message.id
                                            << std::endl;
                                    },

                                    [message](
                                        const std::string& error)
                                    {
                                        std::cout
                                            << "[MessageDao] "
                                               "Mark delivered failed "
                                            << message.id
                                            << ": "
                                            << error
                                            << std::endl;
                                    }
                                );
                            },

                            [message](const std::string& error)
                            {
                                std::cout
                                    << "[UserDao] "
                                       "Find sender username failed "
                                    << message.senderId
                                    << ": "
                                    << error
                                    << std::endl;
                            }
                        );
                    }
                },

                [username](const std::string& error)
                {
                    std::cout
                        << "[MessageDao] "
                           "Get offline messages failed for "
                        << username
                        << ": "
                        << error
                        << std::endl;
                }
            );
        },

        [username](const std::string& error)
        {
            std::cout
                << "[UserDao] "
                   "Find user ID failed for "
                << username
                << ": "
                << error
                << std::endl;
        }
    );
}

void ChatController::handleNewMessage(
    const WebSocketConnectionPtr &wsConn,
    std::string &&message,
    const WebSocketMessageType &type)
{
    if (type != WebSocketMessageType::Text)
    {
        return;
    }

    std::cout
        << "[WebSocket] Received: "
        << message
        << std::endl;

    Json::CharReaderBuilder builder;
    Json::Value root;
    JSONCPP_STRING errors;

    std::istringstream stream(message);

    if (!Json::parseFromStream(
            builder,
            stream,
            &root,
            &errors))
    {
        std::cout
            << "[WebSocket] Invalid JSON: "
            << errors
            << std::endl;

        return;
    }

    if (!root.isMember("type") ||
        root["type"].asString() != "message")
    {
        return;
    }

    if (!root.isMember("from") ||
        !root.isMember("to") ||
        !root.isMember("content"))
    {
        return;
    }

    std::string from =
        root["from"].asString();

    std::string to =
        root["to"].asString();

    std::string content =
        root["content"].asString();


    // 1. 查询发送者 ID
    userDao.findUserId(
        from,

        [this, to, content, from, wsConn]
        (long long senderId)
        {
            // 2. 查询接收者 ID
            userDao.findUserId(
                to,

                [this, from, to, content,
                 senderId, wsConn]
                (long long receiverId)
                {
                    // 3. 判断两人是否为好友
                    friendDao.isFriend(
                        senderId,
                        receiverId,

                        [this, from, to, content,
                         senderId, receiverId,
                         wsConn]
                        (bool isFriend)
                        {
                            // 不是好友
                            if (!isFriend)
                            {
                                Json::Value response;

                                response["type"] =
                                    "error";

                                response["message"] =
                                    "你们不是好友，无法发送消息";

                                Json::StreamWriterBuilder
                                    writer;

                                wsConn->send(
                                    Json::writeString(
                                        writer,
                                        response
                                    )
                                );

                                std::cout
                                    << "[WebSocket] "
                                    << from
                                    << " -> "
                                    << to
                                    << " rejected: "
                                    << "not friends"
                                    << std::endl;

                                return;
                            }


                            // 4. 是好友，保存消息
                            messageDao.saveMessage(
                                senderId,
                                receiverId,
                                content,

                                [this, from, to, content]()
                                {
                                    std::lock_guard<std::mutex>
                                        lock(mutex);

                                    auto it =
                                        onlineUsers.find(to);

                                    // 5. 对方不在线
                                    if (it == onlineUsers.end())
                                    {
                                        Json::Value response;

                                        response["type"] =
                                            "error";

                                        response["message"] =
                                            "User is offline";

                                        Json::StreamWriterBuilder
                                            writer;

                                        auto senderIt =
                                            onlineUsers.find(from);

                                        if (senderIt !=
                                            onlineUsers.end())
                                        {
                                            senderIt->second->send(
                                                Json::writeString(
                                                    writer,
                                                    response
                                                )
                                            );
                                        }

                                        return;
                                    }


                                    // 6. 对方在线，转发消息
                                    Json::Value response;

                                    response["type"] =
                                        "message";

                                    response["from"] =
                                        from;

                                    response["to"] =
                                        to;

                                    response["content"] =
                                        content;

                                    Json::StreamWriterBuilder
                                        writer;

                                    it->second->send(
                                        Json::writeString(
                                            writer,
                                            response
                                        )
                                    );

                                    std::cout
                                        << "[WebSocket] "
                                        << "Saved and forwarded: "
                                        << from
                                        << " -> "
                                        << to
                                        << std::endl;
                                },

                                [from, to]
                                (const std::string& error)
                                {
                                    std::cout
                                        << "[MessageDao] "
                                        << "Save failed: "
                                        << from
                                        << " -> "
                                        << to
                                        << " : "
                                        << error
                                        << std::endl;
                                }
                            );
                        },

                        [wsConn]
                        (const std::string& error)
                        {
                            Json::Value response;

                            response["type"] =
                                "error";

                            response["message"] =
                                error;

                            Json::StreamWriterBuilder
                                writer;

                            wsConn->send(
                                Json::writeString(
                                    writer,
                                    response
                                )
                            );
                        }
                    );
                },

                [to]
                (const std::string& error)
                {
                    std::cout
                        << "[UserDao] "
                        << "Receiver lookup failed: "
                        << to
                        << " : "
                        << error
                        << std::endl;
                }
            );
        },

        [from]
        (const std::string& error)
        {
            std::cout
                << "[UserDao] "
                << "Sender lookup failed: "
                << from
                << " : "
                << error
                << std::endl;
        }
    );
}

void ChatController::handleConnectionClosed(
    const WebSocketConnectionPtr &wsConn)
{
    std::lock_guard<std::mutex> lock(mutex);

    for (auto it = onlineUsers.begin();
         it != onlineUsers.end();
         ++it)
    {
        if (it->second == wsConn)
        {
            std::cout
                << "[WebSocket] User disconnected: "
                << it->first
                << std::endl;

            onlineUsers.erase(it);
            break;
        }
    }
}