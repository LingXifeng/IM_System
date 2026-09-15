#pragma once

#include "../dao/FriendDao.h"
#include "../dao/UserDao.h"

class FriendController
{
public:
    // 添加好友
    static void addFriend(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    // 删除好友
    static void deleteFriend(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    // 获取好友列表
    static void getFriendList(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

private:
    static FriendDao friendDao;
    static UserDao userDao;
};