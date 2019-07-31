#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setWindowIcon(QIcon("./favicon.png"));
    w.show();
    QIcon icon("./favicon.png");


    a.setWindowIcon(icon);

    return a.exec();
}
