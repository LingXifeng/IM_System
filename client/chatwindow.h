#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QString>

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
        const QString& username,
        QWidget *parent = nullptr
        );

    ~ChatWindow() override;

private:
    Ui::ChatWindow *ui;
    QString username;
};

#endif // CHATWINDOW_H