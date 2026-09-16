#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QString>
#include <QTimer>
#include <QHash>
#include <QTextCursor>

#include "network/WebSocketClient.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class ChatWindow;
}
QT_END_NAMESPACE

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(
        const QString &username,
        QWidget *parent = nullptr
        );

    ~ChatWindow() override;

private slots:
    void onSendClicked();
    void onAddFriendClicked();
    void onDeleteFriendClicked();
    void onSearchClicked();

private:
    void loadFriendList();

    void scheduleMessageExpiry(
        qint64 messageId,
        const QString& expireAt
        );

    Ui::ChatWindow *ui;
    QString username;
    QString currentFriend;

    WebSocketClient *webSocketClient;

    // messageId -> 对应消息在 QTextEdit 中的文本位置
    QHash<qint64, QTextCursor> expiringMessages;
};

#endif // CHATWINDOW_H