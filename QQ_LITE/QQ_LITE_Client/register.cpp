#include "register.h"
#include "ui_register.h"

Register::Register(QTcpSocket *socket,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Register)
    , socket(socket)
{
    ui->setupUi(this);
    //隐藏警告
    ui->aclabel->setVisible(false);
    ui->paslabel->setVisible(false);
    ui->warning->setVisible(false);
    ui->tokenlabel->setVisible(false);
}

Register::~Register()
{
    delete ui;
}

//发送注册的账号密码和令牌
void Register::sendinformation()
{
    QString data = QString("NEWACCOUNT:%1|%2|%3")
    .arg(account)
        .arg(passwd)
        .arg(token);
    //发送事件
    socket->write(data.toUtf8());
    socket->flush();
    //关闭窗口
    this->close();
}

QString Register::isid()
{
    return token;
}

//注册事件
void Register::on_buttonBox_2_clicked()
{
    //判断账号是否已经填写
    account=ui->newaccountEdit->text();
    if(account.isEmpty())
    {
        ui->aclabel->setVisible(true);
        return;
    }
    ui->aclabel->setVisible(false);
    //判断密码是否填写
    passwd=ui->newpasswdEdit->text();
    if(passwd.isEmpty())
    {
        ui->paslabel->setVisible(true);
        return;
    }
    ui->paslabel->setVisible(false);
    QString npasswd=ui->npasswdEdit->text();
    //判断再次输入的密码是否与原密码相同
    if(npasswd!=passwd)
    {
        ui->warning->setVisible(true);
        return;
    }
    ui->warning->setVisible(false);
    token=ui->tokenEdit->text();
    //判断令牌是否填写并且令牌长度为6
    if(token.isEmpty()||token.length()!=6)
    {
        ui->tokenlabel->setVisible(true);
        return;
    }
    else if(token.length()==6)
    {
        for(int i=0;i<6;i++)
        {
            QChar ch=token.at(i);
            //判断令牌是否是纯数字
            if(!ch.isDigit())
            {
                ui->tokenlabel->setVisible(true);
                return;
            }
        }
        ui->tokenlabel->setVisible(false);
    }
    sendinformation();
}


void Register::on_pushButton_clicked()
{
    this->close();
}

