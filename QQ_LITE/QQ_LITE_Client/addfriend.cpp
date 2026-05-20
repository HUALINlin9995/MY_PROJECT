#include "addfriend.h"
#include "ui_addfriend.h"

AddFriend::AddFriend(QString user,QTcpSocket *socket,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddFriend)
    ,socket(socket)
    ,user(user)
{
    ui->setupUi(this);
}

AddFriend::~AddFriend()
{
    delete ui;
}

void AddFriend::on_pushButton_clicked()
{
    QString friends=ui->AddEdit->text();
    QString add=QString("ADD_FRIEND:%1|%2").arg(user).arg(friends);
    socket->write(add.toUtf8());
    socket->flush();
    this->close();
}


void AddFriend::on_pushButton_2_clicked()
{
    this->close();
}

