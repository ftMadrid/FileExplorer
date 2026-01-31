#include "notepad.h"
#include "ui_notepad.h"
#include <QFileDialog> // to open the search window
#include <QFile>        // to manage the file
#include <QTextStream>  // to read the text
#include <QMessageBox>

Notepad::Notepad(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Notepad)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setFixedSize(800, 600);
}

Notepad::~Notepad()
{
    delete ui;
}

void Notepad::on_actionLoad_File_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open file");

    if (fileName.isEmpty()) return;

    currentFilePath = fileName; // save the path!!!!!!!

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream input(&file);
        ui->plainTextEdit->setPlainText(input.readAll());
        file.close();
        this->setWindowTitle("Notepad - " + fileName);
    }
}


void Notepad::on_actionSave_File_triggered()
{
    // if the file is new we save as
    if (currentFilePath.isEmpty()) {
        currentFilePath = QFileDialog::getSaveFileName(this, "Save File", "", "File (*.txt)");
    }

    // just leave if the user cancel
    if (currentFilePath.isEmpty()) return;

    // save in the origin path
    QFile file(currentFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "The file cannot be saved!");
        return;
    }

    QTextStream exit(&file);
    exit << ui->plainTextEdit->toPlainText();
    file.close();

    // an announced to say that is saved (yeah yeah)
    this->statusBar()->showMessage("File saved.", 2000);
}

