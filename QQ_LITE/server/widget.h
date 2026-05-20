#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSet>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QTextEdit>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QMap>
#include "caccount.h"
#include "daccount.h"
#define PORT  3389

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    bool issave;
    QTcpServer *server;
    QSet<QTcpSocket *> clientSockets;
    QSet<QString> onlineUsers;
    //映射socket到用户ID
    QMap<QTcpSocket*,QString> socketToUserID;
    QMap<QString,QString*> pendingMsgPtrs;

private slots:
    void newclinethandler();

    void receiveMessage();

    void on_deleteButton_clicked();

    void on_CheckButton_clicked();

    void clientDisconnected();

private:
    Ui::Widget *ui;
    QString userid;
    void savedate(const QString &account,const QString &passwd,const QString &id);
    void forwardFileInChunks(QTcpSocket* receiverSocket, const QByteArray& fileData);
    void ShowOnlineUser();
    QSqlDatabase db;
    QString receivers;
    QByteArray m_fileDataBuffer;   // 累加的文件数据
    qint64 m_expectedFileSize = -1; // 预期的文件总大小（从头部解析）
    QString m_currentFileName;     // 当前接收的文件名
    QMap<QTcpSocket*, QByteArray> m_pendingForwardData;  // 待转发的文件数据
    QMap<QTcpSocket*, qint64> m_forwardedSize;     // 已转发的字节数
    QMap<QTcpSocket*, qint64> m_totalForwardSize;  // 总需转发的字节数
    QStringList FriendGroup(const QString &account);       //获取用户的好友列表
    void send_A_Message(const QString &message,QTcpSocket *socket);
    void onlineaccount(QTcpSocket *socket);
    void broadcastOnlineUsers();     //广播函数
};
#endif // WIDGET_H
