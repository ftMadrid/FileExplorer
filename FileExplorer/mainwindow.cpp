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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow){
    ui->setupUi(this);

    ui->PrincipalWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->favoritesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    favoritesModel = new QStandardItemModel(this);
    ui->favoritesTreeView->setModel(favoritesModel);
    ui->favoritesTreeView->setHeaderHidden(true);
    ui->favoritesTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // load binary
    manager.loadBinary("System777.bin");

    // refresh for the favorites
    favoriteNodes.clear();
    collectFavorites(manager.root);
    updateFavoritesUI();

    qDebug() << "The data bin is at: " << QDir::currentPath();

    // double click
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

    // click on favorites
    connect(ui->favoritesTreeView, &QTreeView::clicked, this, [=](const QModelIndex &index) {
        Node* node = (Node*)index.data(Qt::UserRole).value<void*>();
        if (node) {
            if (node->isFolder) {
                loadFolder(node);
            } else {
                Notepad *notepad = new Notepad(node, &manager);
                notepad->show();
                if (node->parent) loadFolder(node->parent);
            }
        }
    });

    // for remove click of favorites
    connect(ui->favoritesTreeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::on_favoritesTreeView_customContextMenuRequested);

    // copy action
    QAction* copyShortcut = new QAction(this);
    copyShortcut->setShortcut(QKeySequence::Copy);
    connect(copyShortcut, &QAction::triggered, this, &MainWindow::copyAction);
    this->addAction(copyShortcut);

    // paste action
    QAction* pasteShortcut = new QAction(this);
    pasteShortcut->setShortcut(QKeySequence::Paste);
    connect(pasteShortcut, &QAction::triggered, this, [=]() {
        pasteLogic(currentFolder);
    });
    this->addAction(pasteShortcut);

    // load init folder
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

    item->setText(0, QString::fromStdString(node->name));
    item->setData(0, Qt::UserRole, QVariant::fromValue((void*)node));

    if (node->isFolder) {
        item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    } else {
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    }

    item->setExpanded(true);

    for (Node* child : node->children) {
        drawTree(child, item);
    }
}

void MainWindow::loadFolder(Node* folder, bool storeInHistory) {
    if (!folder || !folder->isFolder) return;

    currentFolder = folder;
    if (storeInHistory) {
        history.addStep(folder);
    }

    ui->textEdit->setText(getPath(folder));
    ui->PrincipalWidget->clear();

    for (Node* child : folder->children) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->PrincipalWidget);
        item->setText(0, QString::fromStdString(child->name));
        item->setData(0, Qt::UserRole, QVariant::fromValue((void*)child));

        if (child->isFolder) {
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }
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

bool MainWindow::isAncestor(Node* potentialAncestor, Node* target) {
    Node* temp = target;
    while (temp) {
        if (temp == potentialAncestor) return true;
        temp = temp->parent;
    }
    return false;
}

void MainWindow::pasteLogic(Node* destination) {
    if (!nodeToCopy || !destination) return;

    if (isAncestor(nodeToCopy, destination)) {
        QMessageBox::warning(this, "Error", "You can't paste a file in the same file!");
        return;
    }
    string inputName = nodeToCopy->name;
    string baseName, extension = "";
    if (!nodeToCopy->isFolder) {
        size_t lastDot = inputName.find_last_of(".");
        if (lastDot != string::npos) {
            baseName = inputName.substr(0, lastDot);
            extension = inputName.substr(lastDot);
        } else { baseName = inputName; }
    } else { baseName = inputName; }

    string finalName = baseName;
    if (manager.findChild(destination, finalName + extension) != nullptr) {
        int counter = 1;
        while (manager.findChild(destination, baseName + " copy " + std::to_string(counter) + extension) != nullptr) {
            counter++;
        }
        finalName = baseName + " copy " + std::to_string(counter);
    }
    finalName += extension;

    // used copyNode to adapt to vector
    Node* pastedNode = manager.copyNode(nodeToCopy, destination);
    pastedNode->name = finalName;

    manager.saveBinary("System777.bin");
    loadFolder(currentFolder, false);
    this->statusBar()->showMessage("Pasted: " + QString::fromStdString(finalName), 2000);
}

void MainWindow::copyAction() {
    QTreeWidgetItem* item = ui->PrincipalWidget->currentItem();
    if (!item) return;

    Node* selectedNode = (Node*)item->data(0, Qt::UserRole).value<void*>();

    if (selectedNode) {
        nodeToCopy = selectedNode;
        this->statusBar()->showMessage("Copied: " + QString::fromStdString(nodeToCopy->name), 2000);
    }
}

void MainWindow::updateFavoritesUI() {
    favoritesModel->clear(); // clean the model

    for (Node* node : favoriteNodes) {
        QStandardItem* item = new QStandardItem(QString::fromStdString(node->name));

        // save the pointer when its clicked
        item->setData(QVariant::fromValue((void*)node), Qt::UserRole);

        if (node->isFolder) {
            item->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
        }

        favoritesModel->appendRow(item);
    }
}

void MainWindow::collectFavorites(Node* node) {
    if (!node) return;
    if (node->isFavorite) {
        favoriteNodes.append(node);
    }
    for (Node* child : node->children) {
        collectFavorites(child);
    }
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
        copyAct->setShortcut(QKeySequence::Copy);
        connect(copyAct, &QAction::triggered, this, &MainWindow::copyAction);

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

        menu.addSeparator();
        QAction* favAct = menu.addAction("Add to Favorites");
        connect(favAct, &QAction::triggered, this, [=]() {
            if (!selectedNode->isFavorite) {
                selectedNode->isFavorite = true; // <--- MARCAMOS EL NODO
                favoriteNodes.append(selectedNode);
                updateFavoritesUI();
                manager.saveBinary("System777.bin"); // <--- GUARDAMOS EL BINARIO
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

void MainWindow::on_favoritesTreeView_customContextMenuRequested(const QPoint &pos) {
    QModelIndex index = ui->favoritesTreeView->indexAt(pos);
    if (!index.isValid()) return;

    Node* selectedNode = (Node*)index.data(Qt::UserRole).value<void*>();
    if (!selectedNode) return;

    QMenu menu(this);
    QAction* removeAct = menu.addAction("Remove from Favorites");

    connect(removeAct, &QAction::triggered, this, [=]() {
        selectedNode->isFavorite = false; // remove the mark
        favoriteNodes.removeAll(selectedNode); // bye of temporal list
        updateFavoritesUI();
        manager.saveBinary("System777.bin");
        this->statusBar()->showMessage("Removed from favorites", 2000);
    });

    menu.exec(ui->favoritesTreeView->viewport()->mapToGlobal(pos));
}

