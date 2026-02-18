#include "mainwindow.h"
#include <iostream>

#include <QApplication>

using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    cout << "Loading the program..." <<endl;
    MainWindow w;
    w.show();
    return a.exec();
}
