#pragma once

#include <functional>
#include <string>

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
};