#include "notepad.h"
#include "ui_notepad.h"
#include <QMessageBox>
#include <QMenu>

Notepad::Notepad(Node* targetNode, FileManager* manager, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Notepad)
    , currentNode(targetNode)
    , fileManager(manager)
{
    ui->setupUi(this);
    this->setFixedSize(800, 600);
    ui->plainTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    if (currentNode) {
        this->setWindowTitle("Editing: " + QString::fromStdString(currentNode->name));
        loadNodeContent();
    }
}

Notepad::~Notepad() {
    delete ui;
}

void Notepad::loadNodeContent() {
    if (currentNode) {
        ui->plainTextEdit->setPlainText(QString::fromStdString(currentNode->content));
    }
}

void Notepad::on_actionSave_File_triggered() {
    if (!currentNode) return;

    // update the nodes content in memory
    currentNode->content = ui->plainTextEdit->toPlainText().toStdString();

    // save the whole tree to the binary file for persistence
    fileManager->saveBinary("System777.bin");

    this->statusBar()->showMessage("File saved!", 3000);
}

void Notepad::handleClose() {
    this->close();
}

void Notepad::on_plainTextEdit_customContextMenuRequested(const QPoint &pos) {
    QMenu menu(this);
    menu.addAction(ui->actionSave_File);
    menu.addSeparator();

    menu.addAction("Close Notepad", this, &Notepad::handleClose);

    menu.exec(ui->plainTextEdit->mapToGlobal(pos));
}

