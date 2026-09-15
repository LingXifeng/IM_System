#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QString>

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

    Ui::ChatWindow *ui;
    QString username;
    QString currentFriend;

    WebSocketClient *webSocketClient;
};

#endif // CHATWINDOW_H