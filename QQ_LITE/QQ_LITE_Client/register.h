#ifndef REGISTER_H
#define REGISTER_H

#include <QDialog>
#include <QDebug>
#include <QTcpSocket>
#include <QMessageBox>

namespace Ui {
class Register;
}

class Register : public QDialog
{
    Q_OBJECT

public:
    explicit Register(QTcpSocket *socket,QWidget *parent = nullptr);
    QString isid();
    ~Register();

private slots:
    //发送注册信息事件
    void sendinformation();
    //注册事件
    void on_pushButton_clicked();

    void on_buttonBox_2_clicked();

private:
    Ui::Register *ui;
    QTcpSocket *socket;
    QString account;
    QString passwd;
    QString token;
};

#endif // REGISTER_H
