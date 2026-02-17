#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <QMainWindow>

namespace Ui {
class Notepad;
}

class Notepad : public QMainWindow
{
    Q_OBJECT

public:
    explicit Notepad(QWidget *parent = nullptr);
    ~Notepad();

private slots:
    void on_actionLoad_File_triggered();
    void on_actionSave_File_triggered();
    void on_plainTextEdit_customContextMenuRequested(const QPoint &pos);

private:
    Ui::Notepad *ui;
    QString currentFilePath;
    void empaquetarCarpeta(QString binFilePath, QString sourceDirPath);
    void desempaquetarBinario(QString binFilePath, QString extractPath);
};

#endif // NOTEPAD_H
