#ifndef DELETEFRIEND_H
#define DELETEFRIEND_H

#include <QDialog>
#include <QTcpSocket>

namespace Ui {
class DeleteFriend;
}

class DeleteFriend : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteFriend(QString user,QTcpSocket *socket,QWidget *parent = nullptr);
    ~DeleteFriend();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    Ui::DeleteFriend *ui;
    QString user;
    QTcpSocket *socket;
};

#endif // DELETEFRIEND_H
