#include "chatwindow.h"
#include "ui_chatwindow.h"

#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>

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

    ui->friendList->addItem("ailice");
    ui->friendList->addItem("Bob");
    ui->friendList->addItem("test003");

    connect(
        ui->sendButton,
        &QPushButton::clicked,
        this,
        &ChatWindow::onSendClicked
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