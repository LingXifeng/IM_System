#pragma once

#include "../model/Message.h"

#include <functional>
#include <string>
#include <vector>

class MessageDao
{
public:
    using SuccessCallback =
        std::function<void()>;

    using SaveSuccessCallback =
        std::function<void(long long messageId)>;

    using ErrorCallback =
        std::function<void(const std::string&)>;

    // 保存消息
    // ttlSeconds:
    // 0  = 永不过期
    // >0 = TTL 秒数
    void saveMessage(
        long long senderId,
        long long receiverId,
        const std::string& content,
        long long ttlSeconds,
        SaveSuccessCallback success,
        ErrorCallback error
    );

    using OfflineMessageCallback =
        std::function<void(const std::vector<Message>&)>;

    using MessageCallback =
        std::function<void(const Message&)>;

    void getMessageById(
        long long messageId,
        MessageCallback success,
        ErrorCallback error
    );

    void getOfflineMessages(
        long long receiverId,
        OfflineMessageCallback success,
        ErrorCallback error
    );

    // 获取两个用户之间的聊天历史
    // 自动过滤已经过期的 TTL 消息
    void getChatHistory(
        long long userId,
        long long friendId,
        OfflineMessageCallback success,
        ErrorCallback error
    );

    void markAsDelivered(
        long long messageId,
        SuccessCallback success,
        ErrorCallback error
    );

    void markAsRead(
        long long messageId,
        SuccessCallback success,
        ErrorCallback error
    );
};