#include "mainwindow.h"
#include <iostream>

#include <QApplication>
#include <QDateTime>

using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString timestamp = QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss");
    cout << timestamp.toStdString() << " - [LOG] Loading the program..." << endl;
    MainWindow w;
    w.show();
    return a.exec();
}
