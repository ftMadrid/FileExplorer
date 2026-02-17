#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDirIterator>
#include <QDataStream>
#include <QMenu>
#include "notepad.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->PrincipalWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_PrincipalWidget_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    menu.addAction("Create file");
    menu.addSeparator();
    menu.addAction("Create directory");
    menu.addSeparator();

    QAction *openNotepad = menu.addAction("Open notepad");
    connect(openNotepad, &QAction::triggered, this, [=]() {
        Notepad *notepad = new Notepad();

        notepad->setAttribute(Qt::WA_DeleteOnClose);
        notepad->show();
    });

    menu.exec(QCursor::pos());
}
