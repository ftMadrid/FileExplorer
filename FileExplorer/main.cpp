#include "mainscreen.h"
#include <iostream>

#include <QApplication>

using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainScreen w;
    w.show();
    cout << "Loading the program..." <<endl;
    return a.exec();
}
