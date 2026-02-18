#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "filemanager.h"
#include <QTreeWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_PrincipalWidget_customContextMenuRequested(const QPoint &pos);

private:
    Ui::MainWindow *ui;
    FileManager manager;
    void refreshUI();
    void drawTree(Node* node, QTreeWidgetItem* visualParent); // recursive draw
};

#endif // MAINWINDOW_H
