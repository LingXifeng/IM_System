#include "chatwindow.h"
#include "ui_chatwindow.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>

ChatWindow::ChatWindow(
    const QString &username,
    QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
    , username(username)
    , webSocketClient(new WebSocketClient(this))
{
    ui->setupUi(this);

    setWindowTitle("IM System");

    ui->chatTitleLabel->setText(
        "欢迎，" + username
        );

    connect(
        ui->sendButton,
        &QPushButton::clicked,
        this,
        &ChatWindow::onSendClicked
        );

    connect(
        ui->addFriendButton,
        &QPushButton::clicked,
        this,
        &ChatWindow::onAddFriendClicked
        );

    connect(
        ui->deleteFriendButton,
        &QPushButton::clicked,
        this,
        &ChatWindow::onDeleteFriendClicked
        );

    connect(
        ui->refreshFriendButton,
        &QPushButton::clicked,
        this,
        &ChatWindow::loadFriendList
        );

    connect(
        ui->searchButton,
        &QPushButton::clicked,
        this,
        &ChatWindow::onSearchClicked
        );

    connect(
        ui->friendList,
        &QListWidget::itemClicked,
        this,
        [this](QListWidgetItem *item)
        {
            if (item == nullptr)
            {
                return;
            }

            currentFriend = item->text();

            ui->chatTitleLabel->setText(
                "正在与 " + currentFriend + " 聊天"
                );

            ui->messageView->clear();

            ui->messageView->append(
                "[系统] 已进入与 " +
                currentFriend +
                " 的聊天"
                );
        }
        );

    connect(
        ui->searchResultList,
        &QListWidget::itemDoubleClicked,
        this,
        [this](QListWidgetItem *item)
        {
            if (item == nullptr)
            {
                return;
            }

            QString friendUsername =
                item->text().trimmed();

            if (friendUsername.isEmpty())
            {
                return;
            }

            if (friendUsername == this->username)
            {
                QMessageBox::information(
                    this,
                    "提示",
                    "不能添加自己为好友"
                    );
                return;
            }

            QNetworkAccessManager *manager =
                new QNetworkAccessManager(this);

            QNetworkRequest request(
                QUrl(
                    "http://127.0.0.1:8080/friends/add"
                    )
                );

            request.setHeader(
                QNetworkRequest::ContentTypeHeader,
                "application/json"
                );

            QJsonObject json;

            json["username"] = this->username;;
            json["friendUsername"] =
                friendUsername;

            QJsonDocument document(json);

            QNetworkReply *reply =
                manager->post(
                    request,
                    document.toJson(
                        QJsonDocument::Compact
                        )
                    );

            connect(
                reply,
                &QNetworkReply::finished,
                this,
                [this, reply]()
                {
                    QByteArray data =
                        reply->readAll();

                    if (reply->error() !=
                        QNetworkReply::NoError)
                    {
                        QMessageBox::warning(
                            this,
                            "添加好友失败",
                            reply->errorString()
                            );

                        reply->deleteLater();
                        return;
                    }

                    QJsonDocument document =
                        QJsonDocument::fromJson(data);

                    if (!document.isObject())
                    {
                        QMessageBox::warning(
                            this,
                            "错误",
                            "服务器返回数据异常"
                            );

                        reply->deleteLater();
                        return;
                    }

                    QJsonObject json =
                        document.object();

                    QString message =
                        json["message"].toString();

                    QMessageBox::information(
                        this,
                        "添加好友",
                        message
                        );

                    loadFriendList();

                    reply->deleteLater();
                }
                );
        }
        );

    connect(
        webSocketClient,
        &WebSocketClient::messageReceived,
        this,
        [this](const QString &message)
        {
            QJsonDocument document =
                QJsonDocument::fromJson(
                    message.toUtf8()
                    );

            if (!document.isObject())
            {
                ui->messageView->append(
                    "[服务器] " + message
                    );

                return;
            }

            QJsonObject json =
                document.object();

            QString type =
                json["type"].toString();

            if (type == "message")
            {
                QString from =
                    json["from"].toString();

                QString content =
                    json["content"].toString();

                ui->messageView->append(
                    "[" + from + "] " + content
                    );
            }
            else if (type == "error")
            {
                QString errorMessage =
                    json["message"].toString();

                ui->messageView->append(
                    "[错误] " + errorMessage
                    );
            }
        }
        );


    connect(
        webSocketClient,
        &WebSocketClient::disconnected,
        this,
        [this]()
        {
            ui->messageView->append(
                "[系统] WebSocket 已断开"
                );
        }
        );

    connect(
        webSocketClient,
        &WebSocketClient::errorOccurred,
        this,
        [this](const QString &error)
        {
            ui->messageView->append(
                "[WebSocket错误] " + error
                );
        }
        );

    webSocketClient->connectToServer(username);
    loadFriendList();
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::onSendClicked()
{
    QString message =
        ui->messageEdit->text().trimmed();

    if (message.isEmpty())
    {
        return;
    }

    QListWidgetItem *item =
        ui->friendList->currentItem();

    if (item == nullptr)
    {
        QMessageBox::warning(
            this,
            "提示",
            "请先选择一个好友"
            );

        return;
    }

    QString targetUsername =
        item->text();

    QJsonObject json;

    json["type"] = "message";
    json["from"] = username;
    json["to"] = targetUsername;
    json["content"] = message;

    QJsonDocument document(json);

    QString jsonMessage =
        QString::fromUtf8(
            document.toJson(QJsonDocument::Compact)
            );

    webSocketClient->sendMessage(jsonMessage);

    ui->messageView->append(
        "[我 → " + targetUsername + "] "
        + message
        );

    ui->messageEdit->clear();
}

void ChatWindow::loadFriendList()
{
    QUrl url(
        "http://127.0.0.1:8080/friends/list?username="
        + QUrl::toPercentEncoding(username)
        );

    QNetworkRequest request(url);

    QNetworkAccessManager *manager =
        new QNetworkAccessManager(this);

    QNetworkReply *reply =
        manager->get(request);

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply]()
        {
            if (reply->error() !=
                QNetworkReply::NoError)
            {
                qDebug()
                << "[FriendList] Error:"
                << reply->errorString();

                reply->deleteLater();
                return;
            }

            QByteArray data =
                reply->readAll();

            qDebug()
                << "[FriendList] Response:"
                << data;

            QJsonDocument document =
                QJsonDocument::fromJson(data);

            if (!document.isObject())
            {
                qDebug()
                << "[FriendList] Invalid JSON";

                reply->deleteLater();
                return;
            }

            QJsonObject root =
                document.object();

            QJsonArray friends =
                root["friends"].toArray();

            ui->friendList->clear();

            for (const QJsonValue &value : friends)
            {
                QJsonObject friendObject =
                    value.toObject();

                QString friendUsername =
                    friendObject["username"]
                        .toString();

                if (!friendUsername.isEmpty())
                {
                    ui->friendList->addItem(
                        friendUsername
                        );
                }
            }

            reply->deleteLater();
        }
        );
}

void ChatWindow::onAddFriendClicked()
{
    bool ok = false;

    QString friendUsername =
        QInputDialog::getText(
            this,
            "添加好友",
            "请输入好友用户名：",
            QLineEdit::Normal,
            "",
            &ok
            );

    if (!ok || friendUsername.trimmed().isEmpty())
    {
        return;
    }

    friendUsername =
        friendUsername.trimmed();

    QNetworkAccessManager *manager =
        new QNetworkAccessManager(this);

    QNetworkRequest request(
        QUrl("http://127.0.0.1:8080/friends/add")
        );

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json"
        );

    QJsonObject json;

    json["username"] = username;
    json["friendUsername"] = friendUsername;

    QJsonDocument document(json);

    QNetworkReply *reply =
        manager->post(
            request,
            document.toJson(QJsonDocument::Compact)
            );

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply]()
        {
            QByteArray data =
                reply->readAll();

            if (reply->error() !=
                QNetworkReply::NoError)
            {
                QMessageBox::warning(
                    this,
                    "添加好友失败",
                    reply->errorString()
                    );

                reply->deleteLater();
                return;
            }

            QJsonDocument document =
                QJsonDocument::fromJson(data);

            if (!document.isObject())
            {
                QMessageBox::warning(
                    this,
                    "错误",
                    "服务器返回数据异常"
                    );

                reply->deleteLater();
                return;
            }

            QJsonObject json =
                document.object();

            QString message =
                json["message"].toString();

            QMessageBox::information(
                this,
                "添加好友",
                message
                );

            loadFriendList();

            reply->deleteLater();
        }
        );
}

void ChatWindow::onDeleteFriendClicked()
{
    QListWidgetItem *item =
        ui->friendList->currentItem();

    if (item == nullptr)
    {
        QMessageBox::warning(
            this,
            "删除好友",
            "请先选择一个好友"
            );
        return;
    }

    QString friendUsername =
        item->text();

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "删除好友",
            "确定要删除好友 " +
                friendUsername +
                " 吗？",
            QMessageBox::Yes |
                QMessageBox::No
            );

    if (result != QMessageBox::Yes)
    {
        return;
    }

    QNetworkAccessManager *manager =
        new QNetworkAccessManager(this);

    QNetworkRequest request(
        QUrl("http://127.0.0.1:8080/friends/delete")
        );

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json"
        );

    QJsonObject json;

    json["username"] = username;
    json["friendUsername"] = friendUsername;

    QJsonDocument document(json);

    QNetworkReply *reply =
        manager->post(
            request,
            document.toJson(QJsonDocument::Compact)
            );

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply]()
        {
            QByteArray data =
                reply->readAll();

            if (reply->error() !=
                QNetworkReply::NoError)
            {
                QMessageBox::warning(
                    this,
                    "删除好友失败",
                    reply->errorString()
                    );

                reply->deleteLater();
                return;
            }

            QJsonDocument document =
                QJsonDocument::fromJson(data);

            if (!document.isObject())
            {
                QMessageBox::warning(
                    this,
                    "错误",
                    "服务器返回数据异常"
                    );

                reply->deleteLater();
                return;
            }

            QJsonObject json =
                document.object();

            QString message =
                json["message"].toString();

            QMessageBox::information(
                this,
                "删除好友",
                message
                );

            loadFriendList();

            reply->deleteLater();
        }
        );
}

void ChatWindow::onSearchClicked()
{
    QString keyword =
        ui->searchEdit->text().trimmed();

    if (keyword.isEmpty())
    {
        QMessageBox::information(
            this,
            "搜索用户",
            "请输入用户名"
            );

        return;
    }

    QNetworkAccessManager *manager =
        new QNetworkAccessManager(this);

    QUrl url(
        "http://127.0.0.1:8080/users/search?username="
        + QUrl::toPercentEncoding(keyword)
        );

    QNetworkRequest request(url);

    QNetworkReply *reply =
        manager->get(request);

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply]()
        {
            if (reply->error() !=
                QNetworkReply::NoError)
            {
                QMessageBox::warning(
                    this,
                    "搜索失败",
                    reply->errorString()
                    );

                reply->deleteLater();
                return;
            }

            QByteArray data =
                reply->readAll();

            qDebug()
                << "[Search] Response:"
                << data;

            QJsonDocument document =
                QJsonDocument::fromJson(data);

            if (!document.isObject())
            {
                QMessageBox::warning(
                    this,
                    "错误",
                    "服务器返回数据异常"
                    );

                reply->deleteLater();
                return;
            }

            QJsonObject root =
                document.object();

            QJsonArray users =
                root["users"].toArray();

            ui->searchResultList->clear();

            for (const QJsonValue &value : users)
            {
                QString searchUsername =
                    value.toString();

                if (searchUsername.isEmpty())
                {
                    continue;
                }

                // 不显示自己
                if (searchUsername == username)
                {
                    continue;
                }

                ui->searchResultList->addItem(
                    searchUsername
                    );
            }

            if (ui->searchResultList->count() == 0)
            {
                ui->searchResultList->addItem(
                    "没有找到用户"
                    );
            }

            reply->deleteLater();
        }
        );
}