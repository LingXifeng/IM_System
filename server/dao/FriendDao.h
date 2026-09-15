#pragma once

#include "../model/Friend.h"

#include <functional>
#include <string>
#include <vector>

class FriendDao
{
public:
    using SuccessCallback =
        std::function<void()>;

    using ErrorCallback =
        std::function<void(const std::string&)>;

    // 添加好友关系
    void addFriend(
        long long userId,
        long long friendId,
        SuccessCallback success,
        ErrorCallback error
    );

    // 删除好友关系
    void deleteFriend(
        long long userId,
        long long friendId,
        SuccessCallback success,
        ErrorCallback error
    );

    using FriendListCallback =
        std::function<void(const std::vector<Friend>&)>;

    // 查询好友列表
    void getFriendList(
        long long userId,
        FriendListCallback success,
        ErrorCallback error
    );

    // 判断两个用户是否为好友
    using IsFriendCallback =
        std::function<void(bool)>;

    void isFriend(
        long long userId,
        long long friendId,
        IsFriendCallback success,
        ErrorCallback error
    );
};