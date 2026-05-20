#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QFileDialog>
#include <fstream>
#include <QComboBox>
#include <QFile>
#include <QDesktopServices>
#include <QMessageBox>
#include <QTcpSocket>
#include <QDateTime>
#include <QTableWidgetItem>
#include "addfriend.h"
#include "friendnotice.h"
#include "deletefriend.h"
#define FILENAME "CHAT_HISTORY.txt"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString user,QTcpSocket *socket,QWidget *parent = nullptr);
    void keyPressEvent(QKeyEvent *k);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event)override;

private slots:
    //发送消息按钮
    void on_sendButton_clicked();
    //消息存储事件
    void Save_Informain(const QString &data);
    //清除当前用户聊天记录
    void on_pushButton_clicked();
    //清除所有用户聊天记录
    void on_purgeButton_clicked();
    //发送文件事件
    void on_sendfileButton_clicked();
    //打开接收文件的文件夹事件
    void on_openfileButton_clicked();
    //添加好友事件
    void on_AddFriendButton_clicked();
    //删除好友事件
    void on_DeleteFriendButton_clicked();

public slots:
    //接收消息事件
    void handleNewMessage(const QByteArray &message);

private:
    Ui::MainWindow *ui;
    bool FileIsEmpty;
    QTcpSocket *socket;
    QString user;
    QByteArray m_fileDataBuffer;   //累加的文件数据
    qint64 m_expectedFileSize = -1; //预期的文件总大小
    QString m_currentFileName;     //当前接收的文件名
    QComboBox *RcomboBox;
    QStringList friendAccount;     //好友列表存储
    QStringList onlineFriends;     //在线好友列表
    void updateReceiverList();      //更新下拉框选项
    void initFriendTable();     //初始化好友表格事件
    void online_users(const QString &message);     //显示在线好友列表
    // void requestFriendData();
    // void addFriendToTable(const QString &friendName);
    // void removeFriendFromTable(const QString &friendName);
    // bool isFriendOnline(const QString &friendName);
};

#endif // MAINWINDOW_H
