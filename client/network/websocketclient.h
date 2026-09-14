#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>

class WebSocketClient : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClient(QObject *parent = nullptr);

    void connectToServer(const QString &username);
    void sendMessage(const QString &message);
    void disconnectFromServer();

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString &message);
    void errorOccurred(const QString &error);

private:
    QWebSocket *socket;
};

#endif // WEBSOCKETCLIENT_H