#include "mainscreen.h"
#include "./ui_mainscreen.h"
#include "notepad.h"

#include <iostream>

using std::cout;
using std::endl;

MainScreen::MainScreen(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainScreen)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setFixedSize(800, 600);
}

MainScreen::~MainScreen()
{
    delete ui;
}


void MainScreen::on_notepad_clicked()
{
    Notepad *nt = new Notepad(this);

    // clean the memory
    nt->setAttribute(Qt::WA_DeleteOnClose);

    nt->show();
}

