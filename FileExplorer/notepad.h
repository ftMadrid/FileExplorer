#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <QMainWindow>
#include "node.h"
#include "filemanager.h"

namespace Ui {
class Notepad;
}

class Notepad : public QMainWindow
{
    Q_OBJECT

public:
    explicit Notepad(Node* targetNode, FileManager* manager, QWidget *parent = nullptr);
    ~Notepad();

private slots:
    void on_actionSave_File_triggered();
    void handleClose();
    void on_plainTextEdit_customContextMenuRequested(const QPoint &pos);

private:
    Ui::Notepad *ui;
    Node* currentNode;
    FileManager* fileManager;
    void loadNodeContent();
};

#endif
