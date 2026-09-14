#include "WebSocketClient.h"

#include <QDebug>
#include <QUrl>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , socket(new QWebSocket)
{
    connect(
        socket,
        &QWebSocket::connected,
        this,
        [this]()
        {
            qDebug() << "[WebSocket] Connected";

            emit connected();
        }
        );

    connect(
        socket,
        &QWebSocket::disconnected,
        this,
        [this]()
        {
            qDebug() << "[WebSocket] Disconnected";

            emit disconnected();
        }
        );

    connect(
        socket,
        &QWebSocket::textMessageReceived,
        this,
        [this](const QString &message)
        {
            qDebug() << "[WebSocket] Received:"
                     << message;

            emit messageReceived(message);
        }
        );

    connect(
        socket,
        &QWebSocket::errorOccurred,
        this,
        [this](QAbstractSocket::SocketError error)
        {
            Q_UNUSED(error);

            QString errorMessage =
                socket->errorString();

            qDebug() << "[WebSocket] Error:"
                     << errorMessage;

            emit errorOccurred(errorMessage);
        }
        );
}

void WebSocketClient::connectToServer(
    const QString &username)
{
    QUrl url(
        "ws://127.0.0.1:8080/ws?username="
        + QUrl::toPercentEncoding(username)
        );

    qDebug() << "[WebSocket] Connecting to:"
             << url;

    socket->open(url);
}

void WebSocketClient::sendMessage(
    const QString &message)
{
    if (socket->state() ==
        QAbstractSocket::ConnectedState)
    {
        socket->sendTextMessage(message);
    }
    else
    {
        qDebug()
        << "[WebSocket] Not connected";
    }
}

void WebSocketClient::disconnectFromServer()
{
    socket->close();
}