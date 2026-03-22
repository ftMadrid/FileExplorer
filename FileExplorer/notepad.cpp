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

    this->addAction(ui->actionSave_File);

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

    currentNode->content = ui->plainTextEdit->toPlainText().toStdString();
    time_t now = std::time(nullptr);
    currentNode->modificationDate = now;

    if (currentNode->parent) {
        currentNode->parent->modificationDate = now;
    }

    fileManager->saveBinary("System777.bin");
    emit fileSaved(currentNode);
    this->statusBar()->showMessage("File saved.", 3000);
}

void Notepad::on_plainTextEdit_customContextMenuRequested(const QPoint &pos) {
    QMenu menu(this);
    menu.addAction(ui->actionSave_File);

    menu.addAction("❌ Close Notepad", this, &Notepad::handleClose);

    menu.exec(ui->plainTextEdit->mapToGlobal(pos));
}

void Notepad::handleClose() {
    this->close();
}

