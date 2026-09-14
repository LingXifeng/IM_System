#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chatwindow.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    connect(ui->loginButton,
            &QPushButton::clicked,
            this,
            &MainWindow::onLoginClicked);

    connect(ui->registerButton,
            &QPushButton::clicked,
            this,
            &MainWindow::onRegisterClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLoginClicked()
{
    sendAuthRequest(
        "http://127.0.0.1:8080/login",
        true
        );
}

void MainWindow::onRegisterClicked()
{
    sendAuthRequest(
        "http://127.0.0.1:8080/register",
        false
        );
}

void MainWindow::sendAuthRequest(
    const QString& url,
    bool login)
{
    QString username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();

    if (username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(
            this,
            "提示",
            "用户名和密码不能为空"
            );
        return;
    }

    QUrl requestUrl(url);

    QNetworkRequest request(requestUrl);

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json"
        );

    QJsonObject json;
    json["username"] = username;
    json["password"] = password;

    QJsonDocument document(json);

    QByteArray data =
        document.toJson(QJsonDocument::Compact);

    QNetworkReply *reply =
        networkManager->post(request, data);

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply, login]()
        {
            QByteArray responseData =
                reply->readAll();

            // 网络请求失败
            if (reply->error() !=
                QNetworkReply::NoError)
            {
                QMessageBox::critical(
                    this,
                    "网络错误",
                    reply->errorString()
                    );

                reply->deleteLater();
                return;
            }

            // 解析 JSON
            QJsonDocument document =
                QJsonDocument::fromJson(responseData);

            if (!document.isObject())
            {
                QMessageBox::critical(
                    this,
                    "错误",
                    "服务器返回的数据格式错误"
                    );

                reply->deleteLater();
                return;
            }

            QJsonObject json =
                document.object();

            QString status =
                json["status"].toString();

            QString message =
                json["message"].toString();

            // 请求成功
            if (status == "success")
            {
                if (login)
                {
                    // 获取当前用户名
                    QString username =
                        ui->usernameEdit->text();

                    // 创建聊天主界面
                    ChatWindow *chatWindow =
                        new ChatWindow(username);

                    // 主界面关闭时自动释放
                    chatWindow->setAttribute(
                        Qt::WA_DeleteOnClose
                        );

                    // 显示聊天主界面
                    chatWindow->show();

                    // 隐藏登录窗口
                    this->hide();
                }
                else
                {
                    // 注册成功
                    QMessageBox::information(
                        this,
                        "注册成功",
                        message
                        );
                }
            }
            else
            {
                // 登录/注册失败
                QMessageBox::warning(
                    this,
                    "操作失败",
                    message
                    );
            }

            reply->deleteLater();
        }
        );
}