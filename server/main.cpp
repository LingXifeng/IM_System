#include <iostream>
#include <fstream>

#include <json/json.h>

#include <drogon/drogon.h>
#include <drogon/orm/DbConfig.h>

#include "controller/AuthController.h"

using namespace drogon;
using namespace drogon::orm;

int main()
{
    
    // 读取配置文件
    

    std::ifstream configFile(
        "config/config.json",
        std::ifstream::binary
    );

    if (!configFile.is_open())
    {
        std::cerr
            << "无法打开 config.json"
            << std::endl;

        return 1;
    }

    Json::Value root;

    Json::CharReaderBuilder builder;

    JSONCPP_STRING errs;

    if (!Json::parseFromStream(
            builder,
            configFile,
            &root,
            &errs))
    {
        std::cerr
            << "解析 config.json 失败: "
            << errs
            << std::endl;

        return 1;
    }

    
    //  读取数据库配置
    

    const Json::Value& db =
        root["database"];

    if (!db.isObject() ||
        !db.isMember("name") ||
        !db.isMember("host") ||
        !db.isMember("port") ||
        !db.isMember("database") ||
        !db.isMember("user") ||
        !db.isMember("password") ||
        !db.isMember("connection_number") ||
        !db.isMember("character_set") ||
        !db.isMember("is_fast"))
    {
        std::cerr
            << "config.json 中 database 配置不完整"
            << std::endl;

        return 1;
    }

    
    // 初始化数据库
    
    try
    {
        MysqlConfig mysqlConfig;

        mysqlConfig.name =
            db["name"].asString();

        mysqlConfig.host =
            db["host"].asString();

        mysqlConfig.port =
            static_cast<unsigned short>(
                db["port"].asInt()
            );

        mysqlConfig.databaseName =
            db["database"].asString();

        mysqlConfig.username =
            db["user"].asString();

        mysqlConfig.password =
            db["password"].asString();

        mysqlConfig.connectionNumber =
            db["connection_number"].asInt();

        mysqlConfig.characterSet =
            db["character_set"].asString();

        mysqlConfig.isFast =
            db["is_fast"].asBool();
        
        mysqlConfig.timeout = 10.0;  // 秒

        app().addDbClient(mysqlConfig);
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "addDbClient 失败: "
            << e.what()
            << std::endl;

        return 1;
    }

    

   

// Hello 测试
app().registerHandler(
    "/hello",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& callback)
    {
        auto resp =
            HttpResponse::newHttpResponse();

        resp->setBody(
            "Hello, IM Server!"
        );

        callback(resp);
    },
    {Get}
);

   
    // 注册接口
    

    app().registerHandler(
        "/register",

        &AuthController::registerUser,

        {Post}
    );

    
    // 启动服务器
    

    app().addListener(
        "0.0.0.0",
        8080
    );

    std::cout
        << "IM Server starting on port 8080..."
        << std::endl;

    app().run();

    return 0;
}