#ifndef CACCOUNT_H
#define CACCOUNT_H

#include <QDialog>
#include <QSqlQuery>

namespace Ui {
class caccount;
}

class caccount : public QDialog
{
    Q_OBJECT

public:
    explicit caccount(QWidget *parent = nullptr);
    ~caccount();

private slots:
    //void on_buttonBox_accepted();

private:
    Ui::caccount *ui;
};

#endif // CACCOUNT_H
