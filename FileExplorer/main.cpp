#include "mainwindow.h"
#include <iostream>
#include <QStyleFactory>

#include <QApplication>

using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("WindowsVista"));
    MainWindow w;
    w.show();
    cout << "Loading the program..." <<endl;
    return a.exec();
}
