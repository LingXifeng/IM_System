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
#include <QDateTime>
#include <QTimer>
#include <QTextCursor>

#include "crypto/CryptoManager.h"
#include "crypto/CryptoConfig.h"

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

            // 切换聊天窗口后，清除旧消息的 TTL 游标
            expiringMessages.clear();

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

                // =====================================================
                // 1. 检查 AES-GCM 加密字段
                // =====================================================
                if (!json.contains("ciphertext") ||
                    !json.contains("nonce") ||
                    !json.contains("tag"))
                {
                    ui->messageView->append(
                        "[错误] 收到的消息缺少加密数据"
                        );

                    qDebug()
                        << "[E2EE] Missing encrypted fields";

                    return;
                }

                // =====================================================
                // 2. Base64 解码
                // =====================================================
                QByteArray ciphertext =
                    QByteArray::fromBase64(
                        json["ciphertext"]
                            .toString()
                            .toLatin1()
                        );

                QByteArray nonce =
                    QByteArray::fromBase64(
                        json["nonce"]
                            .toString()
                            .toLatin1()
                        );

                QByteArray tag =
                    QByteArray::fromBase64(
                        json["tag"]
                            .toString()
                            .toLatin1()
                        );

                if (ciphertext.isEmpty() ||
                    nonce.isEmpty() ||
                    tag.isEmpty())
                {
                    ui->messageView->append(
                        "[错误] 消息加密数据无效"
                        );

                    qDebug()
                        << "[E2EE] Invalid encrypted data";

                    return;
                }

                // =====================================================
                // 3. 获取共享密钥
                // =====================================================
                QByteArray key =
                    CryptoConfig::encryptionKey();

                // =====================================================
                // 4. AES-256-GCM 解密
                // =====================================================
                QByteArray plaintext =
                    CryptoManager::decrypt(
                        ciphertext,
                        key,
                        nonce,
                        tag
                        );

                if (plaintext.isEmpty())
                {
                    ui->messageView->append(
                        "[错误] 消息解密失败"
                        );

                    qDebug()
                        << "[E2EE] Decryption failed";

                    return;
                }

                // =====================================================
                // 5. UTF-8 转换成 Qt 字符串
                // =====================================================
                QString content =
                    QString::fromUtf8(plaintext);

                QString displayText =
                    "[" + from + "] " + content;

                qDebug()
                    << "[E2EE] Message decrypted:"
                    << content;

                qint64 messageId = 0;

                if (json.contains("messageId"))
                {
                    messageId =
                        json["messageId"].toInteger();
                }

                QString expireAt;

                if (json.contains("expireAt"))
                {
                    expireAt =
                        json["expireAt"].toString();
                }

                /*
     * 普通消息：直接追加
     */
                if (messageId <= 0 || expireAt.isEmpty())
                {
                    ui->messageView->append(
                        displayText
                        );
                }
                else
                {
                    /*
         * 带 TTL 的消息：
         * 记录它在 QTextEdit 中的文字位置，
         * 后续到期时通过 QTextCursor 删除。
         */

                    QTextCursor cursor =
                        ui->messageView->textCursor();

                    cursor.movePosition(
                        QTextCursor::End
                        );

                    int startPosition =
                        cursor.position();

                    cursor.insertText(
                        displayText
                        );

                    int endPosition =
                        cursor.position();

                    cursor.insertBlock();

                    QTextCursor messageCursor =
                        cursor;

                    messageCursor.setPosition(
                        startPosition
                        );

                    messageCursor.setPosition(
                        endPosition,
                        QTextCursor::KeepAnchor
                        );

                    expiringMessages.insert(
                        messageId,
                        messageCursor
                        );

                    scheduleMessageExpiry(
                        messageId,
                        expireAt
                        );

                    qDebug()
                        << "[TTL] Tracking message:"
                        << messageId
                        << "expireAt:"
                        << expireAt;
                }

                // =====================================================
                // 6. 收到消息后立即发送 ACK
                // =====================================================
                if (json.contains("messageId"))
                {
                    qint64 messageId =
                        json["messageId"].toInteger();

                    if (messageId > 0)
                    {
                        QJsonObject ack;

                        ack["type"] =
                            "ack";

                        ack["messageId"] =
                            messageId;

                        QString ackMessage =
                            QString(
                                QJsonDocument(ack)
                                    .toJson(
                                        QJsonDocument::Compact
                                        )
                                );

                        webSocketClient->sendMessage(
                            ackMessage
                            );

                        qDebug()
                            << "[ACK] Sent for message:"
                            << messageId;
                    }
                }
            }

            else if (type == "queued")
            {
                QString queuedMessage =
                    json["message"].toString();

                ui->messageView->append(
                    "[系统] " + queuedMessage
                    );
            }

            else if (type == "read")
            {
                qint64 messageId =
                    json["messageId"].toInteger();

                ui->messageView->append(
                    "[系统] 消息 "
                    + QString::number(messageId)
                    + " 已读"
                    );

                qDebug()
                    << "[READ] Message read:"
                    << messageId;
            }

            else if (type == "delivered")
            {
                qint64 messageId =
                    json["messageId"].toInteger();

                ui->messageView->append(
                    "[系统] 消息 "
                    + QString::number(messageId)
                    + " 已送达"
                    );

                qDebug()
                    << "[ACK] Message delivered:"
                    << messageId;
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

    /*
     * 选择消息 TTL
     *
     * 0    = 永不过期
     * 60   = 60 秒
     * 300  = 5 分钟
     * 3600 = 1 小时
     */
    QStringList ttlTexts;
    ttlTexts << "普通消息（永不过期）"
             << "60 秒后过期"
             << "5 分钟后过期"
             << "1 小时后过期";

    QString selected =
        QInputDialog::getItem(
            this,
            "消息有效期",
            "请选择消息有效期：",
            ttlTexts,
            0,
            false
            );

    if (selected.isEmpty())
    {
        return;
    }

    long long ttlSeconds = 0;

    if (selected == "60 秒后过期")
    {
        ttlSeconds = 60;
    }
    else if (selected == "5 分钟后过期")
    {
        ttlSeconds = 300;
    }
    else if (selected == "1 小时后过期")
    {
        ttlSeconds = 3600;
    }

    // 使用 AES-256-GCM 加密消息
    QByteArray plaintext =
        message.toUtf8();

    QByteArray nonce;
    QByteArray tag;

    QByteArray key =
        CryptoConfig::encryptionKey();

    QByteArray ciphertext =
        CryptoManager::encrypt(
            plaintext,
            key,
            nonce,
            tag
            );

    if (ciphertext.isEmpty())
    {
        QMessageBox::warning(
            this,
            "发送失败",
            "消息加密失败"
            );

        return;
    }

    QJsonObject json;

    json["type"] = "message";
    json["from"] = username;
    json["to"] = targetUsername;

    // 发送 AES-GCM 加密后的数据
    json["ciphertext"] =
        QString::fromLatin1(
            ciphertext.toBase64()
            );

    json["nonce"] =
        QString::fromLatin1(
            nonce.toBase64()
            );

    json["tag"] =
        QString::fromLatin1(
            tag.toBase64()
            );

    // TTL 保持原来的逻辑
    json["ttl"] = ttlSeconds;

    QJsonDocument document(json);

    QString jsonMessage =
        QString::fromUtf8(
            document.toJson(
                QJsonDocument::Compact
                )
            );

    qDebug()
        << "[Send]"
        << jsonMessage;

    webSocketClient->sendMessage(jsonMessage);

    ui->messageView->append(
        "[我 → " + targetUsername + "] "
        + message
        );

    ui->messageEdit->clear();
}

void ChatWindow::scheduleMessageExpiry(
    qint64 messageId,
    const QString& expireAt)
{
    QDateTime expireTime =
        QDateTime::fromString(
            expireAt,
            "yyyy-MM-dd HH:mm:ss"
            );

    if (!expireTime.isValid())
    {
        qDebug()
        << "[TTL] Invalid expireAt:"
        << expireAt;

        return;
    }

    QDateTime now =
        QDateTime::currentDateTime();

    qint64 remainingMs =
        now.msecsTo(expireTime);

    if (remainingMs <= 0)
    {
        qDebug()
        << "[TTL] Message already expired:"
        << messageId;

        return;
    }

    qDebug()
        << "[TTL] Message"
        << messageId
        << "will expire in"
        << remainingMs
        << "ms";

    QTimer::singleShot(
        remainingMs,
        this,
        [this, messageId]()
        {
            auto it =
                expiringMessages.find(messageId);

            if (it == expiringMessages.end())
            {
                return;
            }

            QTextCursor cursor =
                it.value();

            /*
     * 1. 删除消息正文
     */
            cursor.removeSelectedText();

            /*
     * 2. 删除消息后面的空白段落
     *
     * insertBlock() 创建的就是这个段落。
     * 当前 cursor 位于消息正文结束位置，
     * deleteChar() 会删除后面的段落分隔符。
     */
            cursor.deleteChar();

            /*
     * 3. 删除记录
     */
            expiringMessages.erase(it);

            qDebug()
                << "[TTL] Message expired:"
                << messageId;
        }
        );
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