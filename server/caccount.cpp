#include "caccount.h"
#include "ui_caccount.h"

caccount::caccount(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::caccount)
{
    ui->setupUi(this);
    QSqlQuery qy("select *from UserInfo");
    while(qy.next())
    {
        QString account=qy.value(0).toString();
        QString passwd=qy.value(1).toString();
        QString id=qy.value(2).toString();
        ui->textEdit->append(QString("账号：%1 密码：%2 UID：%3").arg(account).arg(passwd).arg(id));
    }
}

caccount::~caccount()
{
    delete ui;
}

