#include "ChatController.h"

#include <iostream>
#include <sstream>
#include <mutex>
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
                        // 根据 senderId 查找发送者用户名
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

                                Json::CharReaderBuilder readerBuilder;
                                Json::Value encryptedData;
                                JSONCPP_STRING errors;

                                std::istringstream encryptedStream(
                                    message.content
                                );

                                if (Json::parseFromStream(
                                        readerBuilder,
                                        encryptedStream,
                                        &encryptedData,
                                        &errors)
                                    && encryptedData.isObject())
                                {
                                    response["ciphertext"] =
                                        encryptedData["ciphertext"];

                                    response["nonce"] =
                                        encryptedData["nonce"];

                                    response["tag"] =
                                        encryptedData["tag"];
                                }
                                else
                                {
                                    std::cout
                                        << "[WebSocket] Invalid encrypted "
                                           "offline message: "
                                        << message.id
                                        << std::endl;

                                    return;
                                }

                                response["messageId"] =
                                    static_cast<Json::Int64>(
                                        message.id
                                    );

                                // TTL 消息才发送 expireAt
                                if (!message.expireAt.empty())
                                {
                                    response["expireAt"] =
                                        message.expireAt;
                                }

                                Json::StreamWriterBuilder writer;

                                // 推送给重新上线的用户
                                wsConn->send(
                                    Json::writeString(
                                        writer,
                                        response
                                    )
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

    if (!root.isMember("type"))
    {
        return;
    }

    std::string messageType =
        root["type"].asString();

    // =========================================================
    // ACK：消息已送达
    // =========================================================
    if (messageType == "ack")
    {
        if (!root.isMember("messageId"))
        {
            return;
        }

        long long messageId =
            root["messageId"].asInt64();

        if (messageId <= 0)
        {
            return;
        }

        std::string ackUsername;

        {
            std::lock_guard<std::mutex> lock(mutex);

            for (const auto &entry : onlineUsers)
            {
                if (entry.second == wsConn)
                {
                    ackUsername = entry.first;
                    break;
                }
            }
        }

        if (ackUsername.empty())
        {
            return;
        }

        messageDao.getMessageById(
            messageId,

            [this, wsConn, messageId, ackUsername]
            (const Message &message)
            {
                // ACK 必须由消息接收者发送
                userDao.findUserId(
                    ackUsername,

                    [this, wsConn, messageId, message]
                    (long long userId)
                    {
                        if (userId != message.receiverId)
                        {
                            Json::Value response;

                            response["type"] =
                                "error";

                            response["message"] =
                                "无权确认该消息";

                            Json::StreamWriterBuilder writer;

                            wsConn->send(
                                Json::writeString(
                                    writer,
                                    response
                                )
                            );

                            return;
                        }

                        messageDao.markAsDelivered(
                            messageId,

                            [this, messageId, message]()
                            {
                                userDao.findUsernameById(
                                    message.senderId,

                                    [this, messageId]
                                    (const std::string &senderUsername)
                                    {
                                        std::lock_guard<std::mutex> lock(mutex);

                                        auto it =
                                            onlineUsers.find(
                                                senderUsername
                                            );

                                        if (it ==
                                            onlineUsers.end())
                                        {
                                            return;
                                        }

                                        Json::Value response;

                                        response["type"] =
                                            "delivered";

                                        response["messageId"] =
                                            static_cast<Json::Int64>(
                                                messageId
                                            );

                                        Json::StreamWriterBuilder writer;

                                        it->second->send(
                                            Json::writeString(
                                                writer,
                                                response
                                            )
                                        );

                                        std::cout
                                            << "[WebSocket] "
                                            << "Message delivered: "
                                            << messageId
                                            << std::endl;
                                    },

                                    [](const std::string &error)
                                    {
                                        std::cout
                                            << "[UserDao] "
                                            << "Find sender username failed: "
                                            << error
                                            << std::endl;
                                    }
                                );
                            },

                            [messageId]
                            (const std::string &error)
                            {
                                std::cout
                                    << "[MessageDao] "
                                    << "Mark delivered failed: "
                                    << messageId
                                    << " : "
                                    << error
                                    << std::endl;
                            }
                        );
                    },

                    [](const std::string &error)
                    {
                        std::cout
                            << "[UserDao] "
                            << "Find ACK user ID failed: "
                            << error
                            << std::endl;
                    }
                );
            },

            [wsConn]
            (const std::string &error)
            {
                Json::Value response;

                response["type"] =
                    "error";

                response["message"] =
                    error;

                Json::StreamWriterBuilder writer;

                wsConn->send(
                    Json::writeString(
                        writer,
                        response
                    )
                );
            }
        );

        return;
    }

    // =========================================================
    // READ：消息已读
    // =========================================================
    if (messageType == "read")
    {
        if (!root.isMember("messageId"))
        {
            return;
        }

        long long messageId =
            root["messageId"].asInt64();

        if (messageId <= 0)
        {
            return;
        }

        std::string readUsername;

        {
            std::lock_guard<std::mutex> lock(mutex);

            for (const auto &entry : onlineUsers)
            {
                if (entry.second == wsConn)
                {
                    readUsername = entry.first;
                    break;
                }
            }
        }

        if (readUsername.empty())
        {
            return;
        }

        messageDao.getMessageById(
            messageId,

            [this, wsConn, messageId, readUsername]
            (const Message &message)
            {
                userDao.findUserId(
                    readUsername,

                    [this, wsConn, messageId, message]
                    (long long userId)
                    {
                        if (userId != message.receiverId)
                        {
                            Json::Value response;

                            response["type"] =
                                "error";

                            response["message"] =
                                "无权标记该消息为已读";

                            Json::StreamWriterBuilder writer;

                            wsConn->send(
                                Json::writeString(
                                    writer,
                                    response
                                )
                            );

                            return;
                        }

                        messageDao.markAsRead(
                            messageId,

                            [this, messageId, message]()
                            {
                                userDao.findUsernameById(
                                    message.senderId,

                                    [this, messageId]
                                    (const std::string &senderUsername)
                                    {
                                        std::lock_guard<std::mutex> lock(mutex);

                                        auto it =
                                            onlineUsers.find(
                                                senderUsername
                                            );

                                        if (it ==
                                            onlineUsers.end())
                                        {
                                            return;
                                        }

                                        Json::Value response;

                                        response["type"] =
                                            "read";

                                        response["messageId"] =
                                            static_cast<Json::Int64>(
                                                messageId
                                            );

                                        Json::StreamWriterBuilder writer;

                                        it->second->send(
                                            Json::writeString(
                                                writer,
                                                response
                                            )
                                        );

                                        std::cout
                                            << "[WebSocket] "
                                            << "Message read: "
                                            << messageId
                                            << std::endl;
                                    },

                                    [](const std::string &error)
                                    {
                                        std::cout
                                            << "[UserDao] "
                                            << "Find sender username failed: "
                                            << error
                                            << std::endl;
                                    }
                                );
                            },

                            [messageId]
                            (const std::string &error)
                            {
                                std::cout
                                    << "[MessageDao] "
                                    << "Mark read failed: "
                                    << messageId
                                    << " : "
                                    << error
                                    << std::endl;
                            }
                        );
                    },

                    [](const std::string &error)
                    {
                        std::cout
                            << "[UserDao] "
                            << "Find read user ID failed: "
                            << error
                            << std::endl;
                    }
                );
            },

            [wsConn]
            (const std::string &error)
            {
                Json::Value response;

                response["type"] =
                    "error";

                response["message"] =
                    error;

                Json::StreamWriterBuilder writer;

                wsConn->send(
                    Json::writeString(
                        writer,
                        response
                    )
                );
            }
        );

        return;
    }

    // =========================================================
    // 普通消息
    // =========================================================
    if (messageType != "message")
    {
        return;
    }

    // 必须包含加密字段
    if (!root.isMember("from") ||
        !root.isMember("to") ||
        !root.isMember("ciphertext") ||
        !root.isMember("nonce") ||
        !root.isMember("tag"))
    {
        return;
    }

    std::string from =
        root["from"].asString();

    std::string to =
        root["to"].asString();

    std::string ciphertext =
        root["ciphertext"].asString();

    std::string nonce =
        root["nonce"].asString();

    std::string tag =
        root["tag"].asString();

    // =========================================================
    // 将密文、nonce、tag 封装成 JSON 字符串
    // 存入 messages.content
    // =========================================================
    Json::Value encryptedData;

    encryptedData["ciphertext"] =
        ciphertext;

    encryptedData["nonce"] =
        nonce;

    encryptedData["tag"] =
        tag;

    Json::StreamWriterBuilder encryptedWriter;

    std::string encryptedContent =
        Json::writeString(
            encryptedWriter,
            encryptedData
        );

    // =========================================================
    // TTL
    // =========================================================
    long long ttlSeconds = 0;

    if (root.isMember("ttl"))
    {
        if (!root["ttl"].isInt64() &&
            !root["ttl"].isInt())
        {
            Json::Value response;

            response["type"] =
                "error";

            response["message"] =
                "ttl 必须是整数";

            Json::StreamWriterBuilder writer;

            wsConn->send(
                Json::writeString(
                    writer,
                    response
                )
            );

            return;
        }

        ttlSeconds =
            root["ttl"].asInt64();

        if (ttlSeconds < 0)
        {
            Json::Value response;

            response["type"] =
                "error";

            response["message"] =
                "ttl 不能为负数";

            Json::StreamWriterBuilder writer;

            wsConn->send(
                Json::writeString(
                    writer,
                    response
                )
            );

            return;
        }
    }

    // =========================================================
    // 1. 查询发送者 ID
    // =========================================================
    userDao.findUserId(
        from,

        [this,
         to,
         from,
         encryptedContent,
         wsConn,
         ttlSeconds]
        (long long senderId)
        {
            // =================================================
            // 2. 查询接收者 ID
            // =================================================
            userDao.findUserId(
                to,

                [this,
                 from,
                 to,
                 encryptedContent,
                 senderId,
                 wsConn,
                 ttlSeconds]
                (long long receiverId)
                {
                    // =========================================
                    // 3. 判断两人是否为好友
                    // =========================================
                    friendDao.isFriend(
                        senderId,
                        receiverId,

                        [this,
                         from,
                         to,
                         encryptedContent,
                         senderId,
                         receiverId,
                         wsConn,
                         ttlSeconds]
                        (bool isFriend)
                        {
                            if (!isFriend)
                            {
                                Json::Value response;

                                response["type"] =
                                    "error";

                                response["message"] =
                                    "你们不是好友，无法发送消息";

                                Json::StreamWriterBuilder writer;

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

                            // =================================
                            // 4. 保存消息
                            //
                            // 注意：
                            // saveMessage 这里只调用一次
                            // =================================
                            messageDao.saveMessage(
                                senderId,
                                receiverId,
                                encryptedContent,
                                ttlSeconds,

                                [this,
                                 from,
                                 to,
                                 wsConn]
                                (long long messageId)
                                {
                                    // =================================
                                    // 根据 messageId 查询刚保存的消息
                                    // 获取数据库实际生成的 expireAt
                                    // =================================
                                    messageDao.getMessageById(
                                        messageId,

                                        [this,
                                         from,
                                         to,
                                         messageId]
                                        (const Message& savedMessage)
                                        {
                                            std::lock_guard<std::mutex>
                                                lock(mutex);

                                            auto it =
                                                onlineUsers.find(to);

                                            // =================================
                                            // 对方不在线
                                            // =================================
                                            if (it ==
                                                onlineUsers.end())
                                            {
                                                Json::Value response;

                                                response["type"] =
                                                    "queued";

                                                response["message"] =
                                                    "消息已保存，对方当前不在线";

                                                Json::StreamWriterBuilder writer;

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

                                                std::cout
                                                    << "[WebSocket] "
                                                    << "User offline, "
                                                    << "message queued: "
                                                    << from
                                                    << " -> "
                                                    << to
                                                    << std::endl;

                                                return;
                                            }

                                            // =================================
                                            // 解析数据库中的加密 JSON
                                            // =================================
                                            Json::CharReaderBuilder
                                                readerBuilder;

                                            Json::Value encryptedData;

                                            JSONCPP_STRING errors;

                                            std::istringstream
                                                encryptedStream(
                                                    savedMessage.content
                                                );

                                            if (!Json::parseFromStream(
                                                    readerBuilder,
                                                    encryptedStream,
                                                    &encryptedData,
                                                    &errors)
                                                || !encryptedData.isObject())
                                            {
                                                std::cout
                                                    << "[WebSocket] "
                                                    << "Invalid encrypted "
                                                    << "message: "
                                                    << messageId
                                                    << std::endl;

                                                return;
                                            }

                                            // =================================
                                            // 在线转发
                                            // =================================
                                            Json::Value response;

                                            response["type"] =
                                                "message";

                                            response["from"] =
                                                from;

                                            response["to"] =
                                                to;

                                            response["ciphertext"] =
                                                encryptedData["ciphertext"];

                                            response["nonce"] =
                                                encryptedData["nonce"];

                                            response["tag"] =
                                                encryptedData["tag"];

                                            response["messageId"] =
                                                static_cast<Json::Int64>(
                                                    messageId
                                                );

                                            // TTL 消息才发送 expireAt
                                            if (!savedMessage.expireAt.empty())
                                            {
                                                response["expireAt"] =
                                                    savedMessage.expireAt;
                                            }

                                            Json::StreamWriterBuilder writer;

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

                                        [this,
                                         wsConn,
                                         messageId]
                                        (const std::string& error)
                                        {
                                            Json::Value response;

                                            response["type"] =
                                                "error";

                                            response["message"] =
                                                "查询保存后的消息失败: "
                                                + error;

                                            Json::StreamWriterBuilder writer;

                                            wsConn->send(
                                                Json::writeString(
                                                    writer,
                                                    response
                                                )
                                            );

                                            std::cout
                                                << "[MessageDao] "
                                                << "Get saved message failed: "
                                                << messageId
                                                << " : "
                                                << error
                                                << std::endl;
                                        }
                                    );
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

                            Json::StreamWriterBuilder writer;

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