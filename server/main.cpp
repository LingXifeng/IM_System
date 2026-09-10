#include<iostream>
#include<drogon/drogon.h>
#include <fstream>
#include <json/json.h>


using namespace drogon;
using namespace drogon::orm;

int main() {
     std::ifstream configFile("config.json", std::ifstream::binary);
    if (!configFile.is_open()) {
        std::cerr << "无法打开 config.json" << std::endl;
        return 1;
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    if (!Json::parseFromStream(builder, configFile, &root, &errs)) {
        std::cerr << "解析 config.json 失败: " << errs << std::endl;
        return 1;
    }
    const Json::Value& db = root["database"];
    if (!db.isObject() ||
        !db.isMember("host") || !db.isMember("port") ||
        !db.isMember("database") || !db.isMember("user") ||
        !db.isMember("password")) {
        std::cerr << "config.json 中 database 配置不完整" << std::endl;
        return 1;
    }
    try {
    app().createDbClient(
        "mysql",                                          // 数据库类型
        db["host"].asString(),                             // 主机地址
        static_cast<unsigned short>(db["port"].asInt()),   // 端口
        db["database"].asString(),                         // 数据库名
        db["user"].asString(),                             // 用户名
        db["password"].asString(),                         // 密码
        4,                                                 // 连接池大小
        "utf8mb4",                                         // 字符集
        "im_client",                                       // 客户端名称
        false                                              // 不使用 FastDbClient
    );
} catch (const std::exception& e) {
    std::cerr << "createDbClient 失败: " << e.what() << std::endl;
    return 1;
}


    
    drogon::app().registerHandler("/hello",[](const drogon::HttpRequestPtr &req,std::function<void (const drogon::HttpResponsePtr &)> &&callback){
        auto resp=drogon::HttpResponse::newHttpResponse();
        resp->setBody("Hello, IM Server!");
        callback(resp);
    }, {drogon::Get});
    drogon::app().registerHandler("/register",[](const drogon::HttpRequestPtr &req,std::function<void (const drogon::HttpResponsePtr &)> &&callback){
        auto jsonPtr=req->getJsonObject();
        if(!jsonPtr)
        {
            auto resp=drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid JSON");
            callback(resp);
            return;
        }
        if (!(*jsonPtr).isMember("username") || !(*jsonPtr).isMember("password"))
        {
            auto resp=drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Missing username or password");
            callback(resp);
            return;
        }
        std::string username = (*jsonPtr)["username"].asString();
        std::string password = (*jsonPtr)["password"].asString();


         auto clientPtr = app().getDbClient("im_client");
            if (!clientPtr) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody("Database client unavailable");
                callback(resp);
                return;
            }

            
            clientPtr->execSqlAsync(
                "INSERT INTO users (username, password_hash) VALUES (?, ?)",
                [callback](const Result &result) {
                    Json::Value respJson;
                    respJson["status"] = "success";
                    respJson["message"] = "User registered successfully";
                    auto resp = HttpResponse::newHttpJsonResponse(respJson);
                    callback(resp);
                },
                [callback](const DrogonDbException &e) {
                    Json::Value respJson;
                    respJson["status"] = "error";
                    respJson["message"] = std::string("Database error: ") + e.base().what();
                    auto resp = HttpResponse::newHttpJsonResponse(respJson);
                    resp->setStatusCode(k500InternalServerError);
                    callback(resp);
                },
                username,   
                password    
            );
        },
        {Post}
    );
      
        
      /*   auto clientPtr = app().getDbClient("im_client");
        if (!clientPtr) {
            std::cerr << "获取 DbClient 失败" << std::endl;
            app().quit();
            return 0;
        }
         clientPtr->execSqlAsync(
            "INSERT INTO users (username, password_hash) VALUES ('test', 'test_hash')",
            [](const Result &result) {
                std::cout << "插入成功" << std::endl;
            },
            [](const DrogonDbException &e) {
                std::cerr << "插入失败: " << e.base().what() << std::endl;
            }
        ); */
    
   drogon::app().addListener("0.0.0.0",8080);
    app().run();

    return 0;
}
