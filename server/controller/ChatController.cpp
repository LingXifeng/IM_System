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

userDao.findUserId(
    from,

    [this, to, content, from](long long senderId)
    {
        userDao.findUserId(
            to,

            [this, from, to, content, senderId]
            (long long receiverId)
            {
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
                            << "[WebSocket] Saved and forwarded: "
                            << from
                            << " -> "
                            << to
                            << std::endl;
                    },

                    [from, to](const std::string& error)
                    {
                        std::cout
                            << "[MessageDao] Save failed: "
                            << from
                            << " -> "
                            << to
                            << " : "
                            << error
                            << std::endl;
                    }
                );
            },

            [to](const std::string& error)
            {
                std::cout
                    << "[UserDao] Receiver lookup failed: "
                    << to
                    << " : "
                    << error
                    << std::endl;
            }
        );
    },

    [from](const std::string& error)
    {
        std::cout
            << "[UserDao] Sender lookup failed: "
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