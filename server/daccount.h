#ifndef DACCOUNT_H
#define DACCOUNT_H

#include <QDialog>
#include <QSqlQuery>
#include <QMessageBox>

namespace Ui {
class daccount;
}

class daccount : public QDialog
{
    Q_OBJECT

public:
    explicit daccount(QSqlDatabase db,QWidget *parent = nullptr);
    QString UID;
    QSqlQuery query;
    ~daccount();

private slots:
    void deleteaccount();

    void on_buttonBox_accepted();

private:
    Ui::daccount *ui;
};

#endif // DACCOUNT_H
