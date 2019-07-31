#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setWindowIcon(QIcon(":/new/prefix1/favicon.png"));
    w.show();
    QIcon icon(":/new/prefix1/favicon.png");


    a.setWindowIcon(icon);

    return a.exec();
}
