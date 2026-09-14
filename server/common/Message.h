#pragma once

#include <string>

struct Message
{
    long long id = 0;

    long long senderId = 0;

    long long receiverId = 0;

    std::string content;

    int status = 0;

    std::string createdAt;
};