#ifndef FRIENDNOTICE_H
#define FRIENDNOTICE_H

#include <QDialog>
#include <QTcpSocket>

namespace Ui {
class FriendNotice;
}

class FriendNotice : public QDialog
{
    Q_OBJECT

public:
    explicit FriendNotice(QString user,QString friends,QTcpSocket *socket,QWidget *parent = nullptr);
    ~FriendNotice();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    Ui::FriendNotice *ui;
    QTcpSocket *socket;
    QString friends;
    QString user;
};

#endif // FRIENDNOTICE_H
