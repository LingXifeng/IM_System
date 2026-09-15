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

    using ErrorCallback =
        std::function<void(const std::string&)>;

    void saveMessage(
        long long senderId,
        long long receiverId,
        const std::string& content,
        SuccessCallback success,
        ErrorCallback error
    );

    using OfflineMessageCallback =
        std::function<void(const std::vector<Message>&)>;

    void getOfflineMessages(
        long long receiverId,
        OfflineMessageCallback success,
        ErrorCallback error
    );

    void markAsDelivered(
        long long messageId,
        SuccessCallback success,
        ErrorCallback error
    );
};