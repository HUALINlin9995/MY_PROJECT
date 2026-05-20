#include "master.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/Meme_4.png"));
    master w;
    w.show();
    return a.exec();
}
