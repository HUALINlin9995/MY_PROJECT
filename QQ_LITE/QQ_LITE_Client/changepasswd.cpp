#include "changepasswd.h"
#include "ui_changepasswd.h"

ChangePasswd::ChangePasswd(QTcpSocket *socket,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChangePasswd)
    ,socket(socket)
{
    ui->setupUi(this);
    ui->warninglabel->setVisible(false);
    ui->warning1->setVisible(true);
    ui->warning2->setVisible(false);
}

ChangePasswd::~ChangePasswd()
{
    delete ui;
}

void ChangePasswd::on_pushButton_clicked()
{
    QString ID=ui->IDEdit->text();
    socket->write(ID.toUtf8());
    socket->flush();
    //判断密码是否填写
    passwd=ui->newpasswd->text();
    if(passwd.isEmpty())
    {
        qDebug()<<"未填密码";
        ui->warning1->setVisible(true);
        return;
    }
    ui->warning1->setVisible(false);
    QString npasswd=ui->npasswd->text();
    //判断再次输入的密码是否与原密码相同
    if(npasswd!=passwd)
    {
        ui->warning2->setVisible(true);
        return;
    }
    else if(npasswd==passwd)
    {
        QString data = QString("CHANGEPASSWD:%1|%2").arg(ID).arg(passwd);
        socket->write(data.toUtf8());
        socket->flush();
        this->close();
    }
}

void ChangePasswd::on_pushButton_2_clicked()
{
    this->close();
}

