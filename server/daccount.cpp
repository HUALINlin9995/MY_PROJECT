#include "daccount.h"
#include "ui_daccount.h"

daccount::daccount(QSqlDatabase db,QWidget *parent)
    : QDialog(parent)
    ,query(db)
    , ui(new Ui::daccount)
{
    ui->setupUi(this);
}

void daccount::deleteaccount()
{
    if(UID.isEmpty())
    {
        QMessageBox::warning(this,"警告","UID不能为空！");
        return;
    }
    QString deleteQuery=QString("DELETE FROM QQ_LITE.dbo.UserInfo WHERE UserID=%1").arg(UID);
    if(!query.exec(deleteQuery))
    {
        QMessageBox::warning(this,"警告","删除失败！");
        return;
    }
    QMessageBox::information(this,"提示","删除成功！");
}

daccount::~daccount()
{
    delete ui;
}

void daccount::on_buttonBox_accepted()
{
    UID=ui->IDEdit->text();
    deleteaccount();
}

