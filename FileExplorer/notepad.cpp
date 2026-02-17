#include "notepad.h"
#include "ui_notepad.h"
#include <QFileDialog> // to open the search window
#include <QFile>        // to manage the file
#include <QTextStream>  // to read the text
#include <QMessageBox>
#include <QDirIterator>
#include <QDataStream>

Notepad::Notepad(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Notepad)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setFixedSize(800, 600);
    ui->plainTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
}

Notepad::~Notepad()
{
    delete ui;
}

void Notepad::on_actionLoad_File_triggered() {
    // 1. Preguntar por el archivo .bin
    QString binFile = QFileDialog::getOpenFileName(this, "Abrir archivo empaquetado", "", "Archivo Binario (*.bin)");
    if (binFile.isEmpty()) return;

    // 2. Preguntar dónde queremos extraer todo
    QString extractPath = QFileDialog::getExistingDirectory(this, "Selecciona carpeta de destino");
    if (extractPath.isEmpty()) return;

    // 3. Llamar a la lógica
    desempaquetarBinario(binFile, extractPath);
}


void Notepad::on_actionSave_File_triggered() {
    // 1. Preguntar qué carpeta queremos guardar
    QString sourceDir = QFileDialog::getExistingDirectory(this, "Selecciona la carpeta a empaquetar");
    if (sourceDir.isEmpty()) return;

    // 2. Preguntar dónde guardar el archivo .bin
    QString binFile = QFileDialog::getSaveFileName(this, "Guardar como...", "", "Archivo Binario (*.bin)");
    if (binFile.isEmpty()) return;

    // 3. Llamar a la lógica
    empaquetarCarpeta(binFile, sourceDir);
}

void Notepad::empaquetarCarpeta(QString binFilePath, QString sourceDirPath) {
    QFile binFile(binFilePath);
    if (!binFile.open(QIODevice::WriteOnly)) return;

    QDataStream out(&binFile);
    out.setVersion(QDataStream::Qt_5_15);

    QDirIterator it(sourceDirPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFile entryFile(it.filePath());
        if (entryFile.open(QIODevice::ReadOnly)) {
            QString relativePath = QDir(sourceDirPath).relativeFilePath(it.filePath());
            out << relativePath;     // Escribimos nombre
            out << entryFile.readAll(); // Escribimos contenido
            entryFile.close();
        }
    }
    this->statusBar()->showMessage("¡Carpeta empaquetada con éxito!", 3000);
}

void Notepad::desempaquetarBinario(QString binFilePath, QString extractPath) {
    QFile binFile(binFilePath);
    if (!binFile.open(QIODevice::ReadOnly)) return;

    QDataStream in(&binFile);
    while (!in.atEnd()) {
        QString fileName;
        QByteArray fileData;
        in >> fileName >> fileData; // Leemos en el mismo orden

        QString fullPath = extractPath + "/" + fileName;
        QDir().mkpath(QFileInfo(fullPath).absolutePath()); // Crea carpetas si no existen

        QFile outFile(fullPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(fileData);
            outFile.close();
        }
    }
    this->statusBar()->showMessage("¡Extracción completada!", 3000);
}

void Notepad::on_plainTextEdit_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    menu.addAction(ui->actionLoad_File); // O como se llamen tus acciones
    menu.addSeparator();
    menu.addAction(ui->actionSave_File);

    menu.exec(ui->plainTextEdit->mapToGlobal(pos));
}

