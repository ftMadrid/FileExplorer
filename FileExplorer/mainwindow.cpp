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

void MainWindow::pasteLogic(Node* destination) {
    if (!nodeToCopy || !destination) return;

    string inputName = nodeToCopy->name;
    string baseName, extension = "";

    if (!nodeToCopy->isFolder) {
        size_t lastDot = inputName.find_last_of(".");
        if (lastDot != string::npos) {
            baseName = inputName.substr(0, lastDot);
            extension = inputName.substr(lastDot);
        } else {
            baseName = inputName;
        }
    } else {
        baseName = inputName;
    }

    // just if is pasted in the same folder
    string finalName = baseName + extension;
    int counter = 1;
    while (manager.findChild(destination, finalName) != nullptr) {
        finalName = baseName + " copy " + std::to_string(counter++) + extension;
    }

    // create clone
    Node* pastedNode = new Node(finalName, nodeToCopy->isFolder, destination);
    pastedNode->content = nodeToCopy->content;
    pastedNode->left = manager.copyNode(nodeToCopy->left, pastedNode);

    // insert in the LCRS
    if (!destination->left) {
        destination->left = pastedNode;
    } else {
        Node* tmp = destination->left;
        while (tmp->right) tmp = tmp->right;
        tmp->right = pastedNode;
    }

    manager.saveBinary("System777.bin");
    loadFolder(currentFolder, false);
    this->statusBar()->showMessage("Pasted: " + QString::fromStdString(finalName), 2000);
}

void MainWindow::on_PrincipalWidget_customContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem* currentItem = ui->PrincipalWidget->itemAt(pos);
    QMenu menu(this);
    Node* targetFolder = currentFolder;

    if (currentItem) {
        Node* selectedNode = (Node*)currentItem->data(0, Qt::UserRole).value<void*>();

        if (selectedNode->isFolder) {
            targetFolder = selectedNode;
            QAction* openAct = menu.addAction("Open Folder");
            connect(openAct, &QAction::triggered, this, [=]() { loadFolder(selectedNode); });
        } else {
            QAction* openNotepad = menu.addAction("Open with Notepad");
            connect(openNotepad, &QAction::triggered, this, [=]() {
                Notepad *notepad = new Notepad(selectedNode, &manager);
                notepad->setAttribute(Qt::WA_DeleteOnClose);
                notepad->show();
            });
        }
        menu.addSeparator();

        // copy
        QAction* copyAct = menu.addAction("Copy");
        connect(copyAct, &QAction::triggered, this, [=]() {
            nodeToCopy = selectedNode;
            this->statusBar()->showMessage("Copied: " + QString::fromStdString(nodeToCopy->name), 2000);
        });

        // paste inside something blah blah
        if (nodeToCopy && selectedNode->isFolder) {
            QAction* pasteInAct = menu.addAction("Paste inside '" + QString::fromStdString(selectedNode->name) + "'");
            connect(pasteInAct, &QAction::triggered, this, [=]() { pasteLogic(selectedNode); });
        }
        menu.addSeparator();

        // rename
        QAction* renameAct = menu.addAction("Rename");
        connect(renameAct, &QAction::triggered, this, [=]() {
            bool ok;
            QString inputName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal,
                                                      QString::fromStdString(selectedNode->name), &ok);
            if (ok && !inputName.isEmpty()) {
                string newName = inputName.toStdString();
                if (!selectedNode->isFolder && (newName.length() < 4 || newName.substr(newName.length() - 4) != ".txt")) {
                    newName += ".txt";
                }

                Node* duplicate = manager.findChild(currentFolder, newName);
                if (duplicate != nullptr && duplicate != selectedNode) {
                    QMessageBox::warning(this, "Error", "Maje, ya existe ese nombre aquí.");
                } else {
                    manager.renameNode(selectedNode, newName);
                    manager.saveBinary("System777.bin");
                    loadFolder(currentFolder, false);
                }
            }
        });

        // delete
        QAction* deleteAct = menu.addAction("Delete");
        connect(deleteAct, &QAction::triggered, this, [=]() {
            if (QMessageBox::question(this, "Delete", "Delete '" + QString::fromStdString(selectedNode->name) + "'?") == QMessageBox::Yes) {
                manager.deleteNode(selectedNode);
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder, false);
            }
        });

    } else {
        // click in a blank space
        if (nodeToCopy) {
            QAction* pasteHereAct = menu.addAction("Paste Here");
            connect(pasteHereAct, &QAction::triggered, this, [=]() { pasteLogic(currentFolder); });
            menu.addSeparator();
        }

        // create file
        QAction* createFile = menu.addAction("Create New File");
        connect(createFile, &QAction::triggered, this, [=]() {
            bool ok;
            QString name = QInputDialog::getText(this, "New File", "Name:", QLineEdit::Normal, "", &ok);
            if (ok && !name.isEmpty()) {
                string inputName = name.toStdString();
                string baseName, extension = ".txt";

                size_t lastDot = inputName.find_last_of(".");
                if (lastDot != string::npos) {
                    baseName = inputName.substr(0, lastDot);
                    extension = inputName.substr(lastDot);
                } else {
                    baseName = inputName;
                }

                string finalName = baseName + extension;
                int counter = 1;
                // method when created a file with a existant name
                while (manager.findChild(currentFolder, finalName) != nullptr) {
                    finalName = baseName + " " + std::to_string(counter++) + extension;
                }

                manager.addNode(currentFolder, finalName, false);
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder, false);
            }
        });

        // create folder
        QAction* createDir = menu.addAction("Create New Folder");
        connect(createDir, &QAction::triggered, this, [=]() {
            bool ok;
            QString name = QInputDialog::getText(this, "New Folder", "Name:", QLineEdit::Normal, "", &ok);
            if (ok && !name.isEmpty()) {
                string baseName = name.toStdString();
                string finalName = baseName;
                int counter = 1;
                // method when created a folder with a existant name
                while (manager.findChild(currentFolder, finalName) != nullptr) {
                    finalName = baseName + " " + std::to_string(counter++);
                }

                manager.addNode(currentFolder, finalName, true);
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder, false);
            }
        });
    }
    menu.exec(QCursor::pos());
}

