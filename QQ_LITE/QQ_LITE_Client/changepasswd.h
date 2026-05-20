#ifndef CHANGEPASSWD_H
#define CHANGEPASSWD_H

#include <QDialog>
#include <QTcpSocket>
#include <QMessageBox>

namespace Ui {
class ChangePasswd;
}

class ChangePasswd : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePasswd(QTcpSocket *socket,QWidget *parent = nullptr);
    ~ChangePasswd();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    QString passwd;
    QTcpSocket *socket;
    Ui::ChangePasswd *ui;
};

#endif // CHANGEPASSWD_H
