#include "chatwindow.h"
#include "ui_chatwindow.h"

ChatWindow::ChatWindow(
    const QString& username,
    QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
    , username(username)
{
    ui->setupUi(this);

    setWindowTitle("IM System");

    ui->chatTitleLabel->setText(
        "欢迎，" + username
        );

    ui->friendList->addItem("test001");
    ui->friendList->addItem("test002");
    ui->friendList->addItem("test003");
}

ChatWindow::~ChatWindow()
{
    delete ui;
}