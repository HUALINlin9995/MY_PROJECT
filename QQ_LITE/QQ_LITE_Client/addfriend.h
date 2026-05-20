#ifndef ADDFRIEND_H
#define ADDFRIEND_H

#include <QDialog>
#include <QTcpSocket>
#include <QMessageBox>

namespace Ui {
class AddFriend;
}

class AddFriend : public QDialog
{
    Q_OBJECT

public:
    explicit AddFriend(QString user,QTcpSocket *socket,QWidget *parent = nullptr);
    ~AddFriend();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::AddFriend *ui;
    QTcpSocket *socket;
    QString user;
};

#endif // ADDFRIEND_H
