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
#include <QDateTime>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow){
    ui->setupUi(this);

    ui->PrincipalWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->favoritesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    favoritesModel = new QStandardItemModel(this);
    ui->favoritesTreeView->setModel(favoritesModel);
    ui->favoritesTreeView->setHeaderHidden(true);
    ui->favoritesTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->favoritesTreeView->setRootIsDecorated(false);

    ui->favoritesTreeView->setIndentation(7);
    ui->verticalLayout->setSpacing(8);

    ui->metaDataTreeWidget->setRootIsDecorated(false);
    ui->metaDataTreeWidget->setIndentation(6);
    ui->metaDataTreeWidget->setColumnCount(2);
    ui->metaDataTreeWidget->setHeaderHidden(true);

    // load binary
    manager.loadBinary("System777.bin");

    Node* trashCheck = manager.findChild(manager.root, ".trash");
    if (!trashCheck) {
        manager.addNode(manager.root, ".trash", true);
    }

    // refresh for the favorites
    favoriteNodes.clear();
    collectFavorites(manager.root);
    updateFavoritesUI();

    qDebug() << "[INIT LOG] Data bin at: " << QDir::currentPath();

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
            updateMetadata(node);
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

    connect(ui->PrincipalWidget, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item) {
        Node* node = (Node*)item->data(0, Qt::UserRole).value<void*>();
        updateMetadata(node);
    });

    // click in favorites bro
    connect(ui->favoritesTreeView, &QTreeView::clicked, this, [=](const QModelIndex &index) {
        Node* node = (Node*)index.data(Qt::UserRole).value<void*>();
        updateMetadata(node);
    });

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

    // a fix of the hist that we alr have
    if (storeInHistory && currentFolder == folder) {
        storeInHistory = false;
    }

    currentFolder = folder;
    if (storeInHistory) {
        history.addStep(folder);
    }

    // update the label for path indicator
    if (folder == manager.root) {
        ui->pathLabel->setText("Root Directory");
    } else if (folder->name == ".trash") {
        ui->pathLabel->setText("Recycle Bin");
    } else {
        ui->pathLabel->setText(QString::fromStdString(folder->name));
    }

    // text bar logic
    if (folder->name == ".trash") {
        ui->textEdit->setText("C:/Recycle Bin");
    } else {
        ui->textEdit->setText(getPath(folder));
    }

    ui->PrincipalWidget->clear();

    for (Node* child : folder->children) {
        if (child->name == ".trash") continue;

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
    if (obj == ui->PrincipalWidget->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QTreeWidgetItem *item = ui->PrincipalWidget->itemAt(mouseEvent->pos());

        if (!item) {
            ui->PrincipalWidget->clearSelection();
            ui->PrincipalWidget->setCurrentItem(nullptr);

            ui->metaDataTreeWidget->clear(); // clear the info of the panel
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
    destination->modificationDate = std::time(nullptr);
    pastedNode->name = finalName;

    pastedNode->creationDate = std::time(nullptr);
    pastedNode->modificationDate = std::time(nullptr);

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
    favoritesModel->clear();

    Node* trashNode = manager.findChild(manager.root, ".trash");
    if (trashNode) {
        QStandardItem* trashItem = new QStandardItem("Recycle Bin");
        trashItem->setData(QVariant::fromValue((void*)trashNode), Qt::UserRole);
        trashItem->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
        // save a flag that makes to didnt delete the recycle bin
        trashItem->setData(true, Qt::UserRole + 1);
        favoritesModel->appendRow(trashItem);
    }

    QStandardItem* spacer = new QStandardItem("");
    spacer->setSelectable(false);
    spacer->setEnabled(false);
    spacer->setSizeHint(QSize(0, 10));
    favoritesModel->appendRow(spacer);

    // insert favorites
    for (Node* node : favoriteNodes) {
        QStandardItem* item = new QStandardItem(QString::fromStdString(node->name));
        item->setData(QVariant::fromValue((void*)node), Qt::UserRole);

        if (node->isFolder) {
            item->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
        }
        favoritesModel->appendRow(item);
    }
}

void MainWindow::updateMetadata(Node* node) {
    ui->metaDataTreeWidget->clear();
    if (!node) return;

    QTreeWidgetItem* spacer = new QTreeWidgetItem(ui->metaDataTreeWidget);
    spacer->setFlags(Qt::NoItemFlags);    // cant touch
    spacer->setSizeHint(0, QSize(0, 15));

    auto addAttr = [this](QString key, QString value) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->metaDataTreeWidget);
        item->setText(0, key);
        item->setText(1, value);

        item->setForeground(0, QBrush(Qt::lightGray));
    };

    addAttr("Name:", QString::fromStdString(node->name));
    addAttr("Type:", node->isFolder ? "Folder" : "File");

    // size calc
    long totalBytes = calculateTotalSize(node);
    addAttr("Size:", formatSize(totalBytes));

    if (node->isFolder) {
        addAttr("Elements:", QString::number(node->children.size()));
    }

    // Fechas con segundos
    QString cDate = QDateTime::fromSecsSinceEpoch(node->creationDate).toString("dd/MM/yyyy hh:mm:ss");
    QString mDate = QDateTime::fromSecsSinceEpoch(node->modificationDate).toString("dd/MM/yyyy hh:mm:ss");

    addAttr("Created:", cDate);
    addAttr("Modified:", mDate);

    ui->metaDataTreeWidget->expandAll();
}

void MainWindow::collectFavorites(Node* node) {
    if (!node) return;

    // [in testing] if the node is the recycle we dont have to check her childs
    if (node->name == ".trash") return;

    if (node->isFavorite) {
        favoriteNodes.append(node);
    }

    for (Node* child : node->children) {
        collectFavorites(child);
    }
}

long MainWindow::calculateTotalSize(Node* node) {
    if (!node) return 0;

    // if is file just return the size
    if (!node->isFolder) {
        return (long)node->content.size();
    }

    // if is folder we calc the size by the childs
    long total = 0;
    for (Node* child : node->children) {
        total += calculateTotalSize(child);
    }
    return total;
}

QString MainWindow::formatSize(long bytes) {
    double size = static_cast<double>(bytes);
    QStringList units = {"Bytes", "KB", "MB", "GB"};
    int unitIndex = 0;

    while (size >= 1024 && unitIndex < units.size() - 1) {
        size /= 1024;
        unitIndex++;
    }

    return QString::number(size, 'f', 1) + " " + units[unitIndex];
}

void MainWindow::on_PrincipalWidget_customContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem* currentItem = ui->PrincipalWidget->itemAt(pos);
    QMenu menu(this);
    Node* targetFolder = currentFolder;

    Node* trash = manager.findChild(manager.root, ".trash");
    bool inTrash = (currentFolder == trash);

    if (inTrash) {
        // recycle mode baby
        if (currentItem) {
            Node* selectedNode = (Node*)currentItem->data(0, Qt::UserRole).value<void*>();

            QAction* restoreAct = menu.addAction("♻️ Restore");
            restoreAct->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::SystemReboot));
            connect(restoreAct, &QAction::triggered, this, [=]() {
                if (selectedNode->originalParent) {
                    // out of the recycle bin
                    trash->children.erase(std::remove(trash->children.begin(), trash->children.end(), selectedNode), trash->children.end());

                    // get into her original father
                    selectedNode->parent = selectedNode->originalParent;
                    selectedNode->parent->children.push_back(selectedNode);

                    manager.saveBinary("System777.bin");
                    loadFolder(trash); // refresh recycle bin!!!!!!!!!!
                }
            });

            QAction* deletePermAct = menu.addAction("❌ Delete Permanently");
            connect(deletePermAct, &QAction::triggered, this, [=]() {
                if (QMessageBox::question(this, "Permanent Delete", "This cannot be undone. Delete?") == QMessageBox::Yes) {
                    manager.deleteNode(selectedNode);
                    manager.saveBinary("System777.bin");
                    loadFolder(trash);
                }
            });
        } else {
            // click funcs
            QAction* restoreAllAct = menu.addAction("♻️ Restore All");
            connect(restoreAllAct, &QAction::triggered, this, [=]() {
                while (!trash->children.empty()) {
                    Node* n = trash->children[0];
                    if (n->originalParent) {
                        n->parent = n->originalParent;
                        n->originalParent->children.push_back(n);
                    }
                    trash->children.erase(trash->children.begin());
                }
                manager.saveBinary("System777.bin");
                loadFolder(trash);
            });

            QAction* emptyTrashAct = menu.addAction("❌ Empty Trash");
            connect(emptyTrashAct, &QAction::triggered, this, [=]() {
                if (QMessageBox::warning(this, "Empty Trash", "Delete all items?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    while (!trash->children.empty()) {
                        manager.deleteNode(trash->children[0]);
                    }
                    manager.saveBinary("System777.bin");
                    loadFolder(trash);
                }
            });
        }
    } else {
        if (currentItem) {
            Node* selectedNode = (Node*)currentItem->data(0, Qt::UserRole).value<void*>();

            if (selectedNode->isFolder) {
                targetFolder = selectedNode;
                QAction* openAct = menu.addAction("Open Folder");
                openAct->setIcon(this->style()->standardIcon(QStyle::SP_DirOpenIcon));
                openAct->setIconVisibleInMenu(true);
                connect(openAct, &QAction::triggered, this, [=]() { loadFolder(selectedNode); });
            } else {
                QAction* openNotepad = menu.addAction("📝 Open with Notepad");
                connect(openNotepad, &QAction::triggered, this, [=]() {
                    Notepad *notepad = new Notepad(selectedNode, &manager);
                    notepad->setAttribute(Qt::WA_DeleteOnClose);
                    notepad->show();
                });
            }
            menu.addSeparator();

            // copy
            QAction* copyAct = menu.addAction("📑 Copy");
            copyAct->setShortcut(QKeySequence::Copy);
            connect(copyAct, &QAction::triggered, this, &MainWindow::copyAction);

            // paste inside something blah blah
            if (nodeToCopy && selectedNode->isFolder) {
                QAction* pasteInAct = menu.addAction("📋 Paste inside '" + QString::fromStdString(selectedNode->name) + "'");
                connect(pasteInAct, &QAction::triggered, this, [=]() { pasteLogic(selectedNode); });
            }
            menu.addSeparator();

            // rename
            QAction* renameAct = menu.addAction("✏️ Rename");
            connect(renameAct, &QAction::triggered, this, [=]() {
                bool ok;
                std::string nameToEdit = selectedNode->name;

                // if is a file we remove the exten [.txt just to evade bugs bro]
                if (!selectedNode->isFolder) {
                    size_t lastDot = nameToEdit.find_last_of(".");
                    if (lastDot != std::string::npos) {
                        nameToEdit = nameToEdit.substr(0, lastDot);
                    }
                }

                QString inputName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal,
                                                          QString::fromStdString(nameToEdit), &ok);

                if (ok && !inputName.isEmpty()) {
                    std::string newName = inputName.toStdString();

                    // if is file we return with the .txt
                    if (!selectedNode->isFolder) {
                        newName += ".txt";
                    }

                    // verify for duplicates
                    Node* duplicate = manager.findChild(currentFolder, newName);
                    if (duplicate != nullptr && duplicate != selectedNode) {
                        QMessageBox::warning(this, "Error", "There is already a file with this name!");
                    } else {
                        manager.renameNode(selectedNode, newName);
                        selectedNode->modificationDate = std::time(nullptr);
                        currentFolder->modificationDate = std::time(nullptr);
                        manager.saveBinary("System777.bin");
                        loadFolder(currentFolder, false);
                        updateMetadata(selectedNode);
                    }
                }
            });

            // delete
            QAction* deleteAct = menu.addAction("🗑️ Delete");
            connect(deleteAct, &QAction::triggered, this, [=]() {
                Node* trash = manager.findChild(manager.root, ".trash");
                if (!selectedNode || selectedNode == manager.root || selectedNode == trash) return;

                selectedNode->isFavorite = false;
                favoriteNodes.removeAll(selectedNode); // remove from the temporal list

                selectedNode->originalParent = selectedNode->parent;
                if (selectedNode->parent) {
                    auto& v = selectedNode->parent->children;
                    v.erase(std::remove(v.begin(), v.end(), selectedNode), v.end());
                }

                if (selectedNode->parent) {
                    selectedNode->parent->modificationDate = std::time(nullptr);
                }
                trash->modificationDate = std::time(nullptr);

                selectedNode->parent = trash;
                trash->children.push_back(selectedNode);

                updateFavoritesUI();

                manager.saveBinary("System777.bin");
                loadFolder(currentFolder, false);
            });

            menu.addSeparator();
            QAction* favAct = menu.addAction("🎖️ Add to Favorites");
            connect(favAct, &QAction::triggered, this, [=]() {
                if (!selectedNode->isFavorite) {
                    selectedNode->isFavorite = true; // mark the node
                    favoriteNodes.append(selectedNode);
                    updateFavoritesUI();
                    manager.saveBinary("System777.bin");
                }
            });

        } else {
            // click in a blank space
            if (nodeToCopy) {
                QAction* pasteHereAct = menu.addAction("📋 Paste Here");
                connect(pasteHereAct, &QAction::triggered, this, [=]() { pasteLogic(currentFolder); });
                menu.addSeparator();
            }

            // create file
            QAction* createFile = menu.addAction("Create New File");
            createFile->setIcon(this->style()->standardIcon(QStyle::SP_FileIcon));
            createFile->setIconVisibleInMenu(true);
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
                    currentFolder->modificationDate = std::time(nullptr);
                    manager.saveBinary("System777.bin");
                    loadFolder(currentFolder, false);
                }
            });

            // create folder
            QAction* createDir = menu.addAction("Create New Folder");
            createDir->setIcon(this->style()->standardIcon(QStyle::SP_DirIcon));
            createDir->setIconVisibleInMenu(true);
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
                    currentFolder->modificationDate = std::time(nullptr);
                    manager.saveBinary("System777.bin");
                    loadFolder(currentFolder, false);
                }
            });
        }
    }
    menu.exec(QCursor::pos());
}

void MainWindow::on_favoritesTreeView_customContextMenuRequested(const QPoint &pos) {
    QModelIndex index = ui->favoritesTreeView->indexAt(pos);
    if (!index.isValid()) return;

    Node* selectedNode = (Node*)index.data(Qt::UserRole).value<void*>();
    if (!selectedNode) return;

    if (index.data(Qt::UserRole + 1).toBool()) return;

    QMenu menu(this);
    QAction* removeAct = menu.addAction("❌ Remove from Favorites");

    connect(removeAct, &QAction::triggered, this, [=]() {
        selectedNode->isFavorite = false; // remove the mark
        favoriteNodes.removeAll(selectedNode); // bye of temporal list
        updateFavoritesUI();
        manager.saveBinary("System777.bin");
        this->statusBar()->showMessage("Removed from favorites", 2000);
    });

    menu.exec(ui->favoritesTreeView->viewport()->mapToGlobal(pos));
}

