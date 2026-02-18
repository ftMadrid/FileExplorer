#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QStyle>
#include "notepad.h"
#include <QDir>
#include <QDebug>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->PrincipalWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    manager.loadBinary("System777.bin");
    qDebug() << "The data bin is at: " << QDir::currentPath();

    // to open with double click [hacking time yeah]
    connect(ui->PrincipalWidget, &QTreeWidget::itemDoubleClicked, this, [=](QTreeWidgetItem* item) {
        Node* node = (Node*)item->data(0, Qt::UserRole).value<void*>();
        if (node) {
            if (node->isFolder) {
                loadFolder(node);
            } else {
                Notepad *notepad = new Notepad(node, &manager);
                notepad->show();
            }
        }
    });

    loadFolder(manager.root, true);
    ui->PrincipalWidget->viewport()->installEventFilter(this);
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

void MainWindow::loadFolder(Node* folder, bool storeInHistory) {
    if (!folder || !folder->isFolder) return;

    // save actual folder
    currentFolder = folder;

    // save in history
    if (storeInHistory) {
        history.addStep(folder);
    }

    // update the textEdit line
    ui->textEdit->setText(getPath(folder));
    // clean the UI
    ui->PrincipalWidget->clear();

    // LCRS logic
    Node* child = folder->left;
    while (child) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->PrincipalWidget);
        item->setText(0, QString::fromStdString(child->name));

        // save the pointer just to know what file it is [without chatgpt my bro]
        item->setData(0, Qt::UserRole, QVariant::fromValue((void*)child));

        // icons
        if (child->isFolder) {
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }

        child = child->right;
    }
}

QString MainWindow::getPath(Node* node) {
    if (!node) return "";
    if (node == manager.root) return "C:/";

    QString parentPath = getPath(node->parent);
    if (!parentPath.endsWith("/")) parentPath += "/";

    return parentPath + QString::fromStdString(node->name);
}

void MainWindow::on_backButton_clicked() {
    if (history.current && history.current->prev) {
        history.current = history.current->prev;
        loadFolder(history.current->folder, false);
    }
}

void MainWindow::on_nextButton_clicked() {
    if (history.current && history.current->next) {
        history.current = history.current->next;
        loadFolder(history.current->folder, false);
    }
}


void MainWindow::on_fatherButton_clicked()
{
    if (currentFolder && currentFolder->parent) {
        loadFolder(currentFolder->parent);
    } else {
        this->statusBar()->showMessage("You are already in the root directory!", 2000);
    }
}


void MainWindow::on_searchButton_clicked() {
    QString path = ui->textEdit->toPlainText().trimmed();

    if (path.startsWith("C:/")) path.remove(0, 3);
    if (path.isEmpty()) {
        loadFolder(manager.root);
        return;
    }

    QStringList parts = path.split("/", Qt::SkipEmptyParts);
    Node* currentNode = manager.root;

    for (int i = 0; i < parts.size(); ++i) {
        Node* nextNode = manager.findChild(currentNode, parts[i].toStdString());

        if (nextNode) {
            // if is not the last part of the roo has to be a folder
            if (i < parts.size() - 1 && !nextNode->isFolder) {
                QMessageBox::warning(this, "Error", parts[i] + " is a file, not a directory!");
                return;
            }
            currentNode = nextNode;
        } else {
            QMessageBox::warning(this, "Error", "Path not found: " + parts[i]);
            return;
        }
    }

    // at the end of the path
    if (currentNode->isFolder) {
        loadFolder(currentNode);
    } else {
        // if is .txt we open with notepad
        Notepad *notepad = new Notepad(currentNode, &manager);
        notepad->show();

        // loaded the folder archive
        if (currentNode->parent) {
            loadFolder(currentNode->parent);
        }
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // if the event is a mouse click
    if (obj == ui->PrincipalWidget->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        // search if is an item selected
        QTreeWidgetItem *item = ui->PrincipalWidget->itemAt(mouseEvent->pos());

        if (!item) {
            // click in the blank space to deselect
            ui->PrincipalWidget->clearSelection();
            ui->PrincipalWidget->setCurrentItem(nullptr);
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::on_PrincipalWidget_customContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem* currentItem = ui->PrincipalWidget->itemAt(pos);
    QMenu menu(this);

    if (currentItem) {
        // click under folder or file
        Node* selectedNode = (Node*)currentItem->data(0, Qt::UserRole).value<void*>();

        if (selectedNode->isFolder) {
            // dir options
            QAction* openAct = menu.addAction("Open");
            connect(openAct, &QAction::triggered, this, [=]() {
                loadFolder(selectedNode); // enter into the folder
            });
            menu.addSeparator();
        } else {
            // file options
            QAction* openNotepad = menu.addAction("Open Notepad");
            connect(openNotepad, &QAction::triggered, this, [=]() {
                Notepad *notepad = new Notepad(selectedNode, &manager);
                notepad->setAttribute(Qt::WA_DeleteOnClose);
                notepad->show();
            });
            menu.addSeparator();
        }

        // common options (in the past two hehe)
        QAction* renameAct = menu.addAction("Rename");
        connect(renameAct, &QAction::triggered, this, [=]() {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal,
                                                    QString::fromStdString(selectedNode->name), &ok);
            if (ok && !newName.isEmpty()) {
                manager.renameNode(selectedNode, newName.toStdString());
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder, false);
            }
        });

        QAction* deleteAct = menu.addAction("Delete");
        connect(deleteAct, &QAction::triggered, this, [=]() {
            auto res = QMessageBox::question(this, "Delete", "Are you sure you want to delete '" +
                                                                 QString::fromStdString(selectedNode->name) + "'?");
            if (res == QMessageBox::Yes) {
                manager.deleteNode(selectedNode);
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder, false);
            }
        });

    } else {
        // click in a blankspace, create things in the actual folder
        // create file
        QAction* createFile = menu.addAction("Create New File");
        connect(createFile, &QAction::triggered, this, [=]() {
            bool ok;
            QString name = QInputDialog::getText(this, "New File", "Name:", QLineEdit::Normal, "", &ok);
            if (ok && !name.isEmpty()) {
                string fName = name.toStdString();
                if (fName.find(".txt") == string::npos) fName += ".txt";
                manager.addNode(currentFolder, fName, false);
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder, false);
            }
        });

        // create folder
        QAction* createDir = menu.addAction("Create New Directory");
        connect(createDir, &QAction::triggered, this, [=]() {
            bool ok;
            QString name = QInputDialog::getText(this, "New Folder", "Name:", QLineEdit::Normal, "", &ok);
            if (ok && !name.isEmpty()) {
                manager.addNode(currentFolder, name.toStdString(), true);
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder, false);
            }
        });
    }

    menu.exec(QCursor::pos());
}

