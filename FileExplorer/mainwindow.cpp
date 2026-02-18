#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QStyle>
#include "notepad.h"
#include <QDir>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->PrincipalWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    manager.loadBinary("System777.bin");
    qDebug() << "The data bin is at: " << QDir::currentPath();
    refreshUI();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// simple func to refresh the UI
void MainWindow::refreshUI() {
    ui->PrincipalWidget->clear();
    drawTree(manager.root, nullptr);
}

void MainWindow::drawTree(Node* node, QTreeWidgetItem* visualParent) {
    if (!node) return;

    QTreeWidgetItem* item;
    if (visualParent) {
        item = new QTreeWidgetItem(visualParent);
    } else {
        item = new QTreeWidgetItem(ui->PrincipalWidget);
    }

    // item config
    item->setText(0, QString::fromStdString(node->name));
    item->setData(0, Qt::UserRole, QVariant::fromValue((void*)node));

    // icons
    if (node->isFolder) {
        item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    } else {
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    }

    item->setExpanded(true); // to see all expanded

    // LCRS recursive
    if (node->left) drawTree(node->left, item);
    if (node->right) drawTree(node->right, visualParent);
}

void MainWindow::on_PrincipalWidget_customContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem* currentItem = ui->PrincipalWidget->itemAt(pos);

    Node* selectedNode = nullptr;
    if (currentItem) {
        selectedNode = (Node*)currentItem->data(0, Qt::UserRole).value<void*>();
    } else {
        selectedNode = manager.root; // default to root if clicking empty space
    }

    QMenu menu(this);

    // if we have a folder, allow creating things inside
    if (selectedNode && selectedNode->isFolder) {

        QAction* createFile = menu.addAction("Create File");
        connect(createFile, &QAction::triggered, this, [=]() {
            bool ok;
            QString name = QInputDialog::getText(this, "New File", "Enter name:", QLineEdit::Normal, "", &ok);
            if (ok && !name.isEmpty()) {
                manager.addNode(selectedNode, name.toStdString(), false);
                manager.saveBinary("System777.bin"); // persistence
                refreshUI(); // eedraw the tree
            }
        });

        QAction* createDir = menu.addAction("Create Directory");
        connect(createDir, &QAction::triggered, this, [=]() {
            bool ok;
            QString name = QInputDialog::getText(this, "New Folder", "Enter name:", QLineEdit::Normal, "", &ok);
            if (ok && !name.isEmpty()) {
                manager.addNode(selectedNode, name.toStdString(), true);
                manager.saveBinary("System777.bin"); // persistence
                refreshUI(); // redraw the tree
            }
        });

        menu.addSeparator();
    }

    if (selectedNode && !selectedNode->isFolder) {
        QAction *openNotepad = menu.addAction("Open Notepad");
        connect(openNotepad, &QAction::triggered, this, [=]() {
            Notepad *notepad = new Notepad(selectedNode, &manager);
            notepad->setAttribute(Qt::WA_DeleteOnClose);
            notepad->show();
        });
    }

    menu.exec(QCursor::pos());
}
