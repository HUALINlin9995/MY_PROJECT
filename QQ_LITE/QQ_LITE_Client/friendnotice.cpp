#include "friendnotice.h"
#include "ui_friendnotice.h"

FriendNotice::FriendNotice(QString user,QString friends,QTcpSocket *socket,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FriendNotice)
    ,socket(socket)
    ,friends(friends)
    ,user(user)
{
    ui->setupUi(this);
    ui->textEdit->append(QString("收到用户%1的好友请求").arg(friends));
}

FriendNotice::~FriendNotice()
{
    delete ui;
}

void FriendNotice::on_pushButton_2_clicked()
{
    this->close();
}


void FriendNotice::on_pushButton_clicked()
{
    qDebug()<<"同意好友请求";
    QString add=QString("ADD_FRIEND_AGREE_TO_THE_REQUEST:%1|%2").arg(friends).arg(user);
    qDebug()<<add;
    socket->write(add.toUtf8());
    socket->flush();
    this->close();
}

