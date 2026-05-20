#include "deletefriend.h"
#include "ui_deletefriend.h"

DeleteFriend::DeleteFriend(QString user,QTcpSocket *socket,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeleteFriend)
    ,user(user)
    ,socket(socket)
{
    ui->setupUi(this);
}

DeleteFriend::~DeleteFriend()
{
    delete ui;
}

void DeleteFriend::on_pushButton_2_clicked()
{
    this->close();
}


void DeleteFriend::on_pushButton_clicked()
{
    QString user1=ui->lineEdit->text();
    QString yy=QString("DELETE_FRIEND:%1|%2").arg(user).arg(user1);
    qDebug()<<yy;
    socket->write(yy.toUtf8());
    socket->flush();
    this->close();
}

