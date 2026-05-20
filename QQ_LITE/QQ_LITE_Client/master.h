#ifndef MASTER_H
#define MASTER_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QTcpSocket>
#include <QMessageBox>
#include <QHostAddress>
#include <QHostInfo>
#include <QKeyEvent>
#include <QNetworkInterface>
#include "register.h"
#include "mainwindow.h"
#include "changepasswd.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class master;
}
QT_END_NAMESPACE

class master : public QWidget
{
    Q_OBJECT

public:
    master(QWidget *parent = nullptr);
    void keyPressEvent(QKeyEvent *k);
    QString User;
    ~master();

private slots:
    //连接事件
    void onHostResolved(const QHostInfo &hostInfo);
    //取消按钮
    void on_cancelButton_clicked();
    //注册按钮
    void on_registerButton_clicked();
    //监听服务器回应事件
    void onReadyRead();
    //重新连接事件
    void on_Reconnect_linkActivated();
    //登录事件
    void on_loginButton_clicked();
    //忘记密码事件
    void on_RTpasswd_linkActivated();

signals:
    void newMessageReceived(const QByteArray &message);

private:
    Ui::master *ui;
    QTcpSocket *socket;
    //存储域名
    QString serverDomain;
    //存储端口
    quint16 serverPort;
    //自己的账号
    //QString User;
    //判断是否在线
    bool isconnect;
};
#endif // MASTER_H
