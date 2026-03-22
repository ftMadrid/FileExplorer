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
#include <QListWidget>
#include <QListView>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // views config
    favoritesModel = new QStandardItemModel(this);
    ui->favoritesTreeView->setModel(favoritesModel);
    ui->favoritesTreeView->setHeaderHidden(true);
    ui->favoritesTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->favoritesTreeView->setIndentation(7);

    ui->metaDataTreeWidget->setColumnCount(2);
    ui->metaDataTreeWidget->setHeaderHidden(true);

    ui->iconsWidget->setViewMode(QListView::IconMode);
    ui->iconsWidget->setIconSize(QSize(64, 64));
    ui->iconsWidget->setResizeMode(QListView::Adjust);
    ui->iconsWidget->setSpacing(10);

    // load data and events filter
    manager.loadBinary("System777.bin");
    ui->iconsWidget->viewport()->installEventFilter(this);
    ui->PrincipalWidget->viewport()->installEventFilter(this);

    // init nodes
    Node* trashCheck = manager.findChild(manager.root, ".trash");
    if (!trashCheck) manager.addNode(manager.root, ".trash", true);

    favoriteNodes.clear();
    collectFavorites(manager.root);
    updateFavoritesUI();

    // double click (icon view)
    connect(ui->PrincipalWidget, &QTreeWidget::itemDoubleClicked, this, [=](QTreeWidgetItem* item) {
        Node* node = (Node*)item->data(0, Qt::UserRole).value<void*>();
        if (node && node->isFolder) loadFolder(node);
        else if (node) {
            Notepad *notepad = new Notepad(node, &manager);
            connect(notepad, &Notepad::fileSaved, this, &MainWindow::updateMetadata);
            notepad->show();
        }
    });

    connect(ui->iconsWidget, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem* item) {
        Node* node = (Node*)item->data(Qt::UserRole).value<void*>();
        if (node && node->isFolder) loadFolder(node);
        else if (node) { Notepad *notepad = new Notepad(node, &manager); notepad->show(); }
    });

    // simple click for metadata
    connect(ui->PrincipalWidget, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item) {
        updateMetadata((Node*)item->data(0, Qt::UserRole).value<void*>());
    });
    connect(ui->iconsWidget, &QListWidget::itemClicked, this, [=](QListWidgetItem* item) {
        updateMetadata((Node*)item->data(Qt::UserRole).value<void*>());
    });

    // favorites
    connect(ui->favoritesTreeView, &QTreeView::clicked, this, [=](const QModelIndex &index) {
        Node* node = (Node*)index.data(Qt::UserRole).value<void*>();
        if (node) {
            updateMetadata(node);
            if (node->isFolder) loadFolder(node);
            else {
                Notepad *notepad = new Notepad(node, &manager);
                notepad->show();
                if (node->parent) loadFolder(node->parent);
            }
        }
    });

    connect(ui->PrincipalWidget, &QTreeWidget::customContextMenuRequested, this, &MainWindow::PrincipalWidget_customContextMenuRequested);
    connect(ui->iconsWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::PrincipalWidget_customContextMenuRequested);
    connect(ui->favoritesTreeView, &QTreeView::customContextMenuRequested, this, &MainWindow::on_favoritesTreeView_customContextMenuRequested);

    // shortcuts
    QAction* copyShortcut = new QAction(this);
    copyShortcut->setShortcut(QKeySequence::Copy);
    connect(copyShortcut, &QAction::triggered, this, &MainWindow::copyAction);
    this->addAction(copyShortcut);

    QAction* pasteShortcut = new QAction(this);
    pasteShortcut->setShortcut(QKeySequence::Paste);
    connect(pasteShortcut, &QAction::triggered, this, [=]() { pasteLogic(currentFolder); });
    this->addAction(pasteShortcut);

    ui->PrincipalWidget->setDragEnabled(true);
    ui->PrincipalWidget->setAcceptDrops(true);
    ui->PrincipalWidget->setDropIndicatorShown(true);
    ui->PrincipalWidget->setDefaultDropAction(Qt::MoveAction);
    ui->PrincipalWidget->setDragDropMode(QAbstractItemView::InternalMove);
    ui->PrincipalWidget->viewport()->setAcceptDrops(true);
    ui->PrincipalWidget->installEventFilter(this);

    // icons widget config
    ui->iconsWidget->setDragEnabled(true);
    ui->iconsWidget->setAcceptDrops(true);
    ui->iconsWidget->setDropIndicatorShown(true);
    ui->iconsWidget->setDefaultDropAction(Qt::MoveAction);
    ui->iconsWidget->setDragDropMode(QAbstractItemView::InternalMove);

    connect(ui->PrincipalWidget->model(), &QAbstractItemModel::rowsMoved, this, [=](const QModelIndex &, int, int, const QModelIndex &parent, int) {
        // cap the destination node
        Node* destNode = nullptr;
        if (parent.isValid()) {
            destNode = (Node*)ui->PrincipalWidget->itemFromIndex(parent)->data(0, Qt::UserRole).value<void*>();
        } else {
            destNode = currentFolder;
        }

        // just to secure the data
        loadFolder(currentFolder, false);
    });

    // for views changes
    if (manager.isIconMode) {
        ui->PrincipalWidget->hide();
        ui->iconsWidget->show();
    } else {
        ui->iconsWidget->hide();
        ui->PrincipalWidget->show();
    }

    loadFolder(manager.root, true);
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
    ui->iconsWidget->clear();

    for (Node* child : folder->children) {
        if (child->name == ".trash") continue;

        QTreeWidgetItem* item = new QTreeWidgetItem(ui->PrincipalWidget);
        item->setText(0, QString::fromStdString(child->name));
        item->setData(0, Qt::UserRole, QVariant::fromValue((void*)child));

        if (child->isFolder) {
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
            // the folders can be dragged and receive files/folders
            item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        } else {
            item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
            // the files can be dragged
            item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
            item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
        }

        // for the icons
        QListWidgetItem* iconItem = new QListWidgetItem(ui->iconsWidget);
        iconItem->setText(QString::fromStdString(child->name));
        iconItem->setData(Qt::UserRole, QVariant::fromValue((void*)child));

        if (child->isFolder) {
            iconItem->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
            iconItem->setFlags(iconItem->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        } else {
            iconItem->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
            iconItem->setFlags(iconItem->flags() | Qt::ItemIsDragEnabled);
            iconItem->setFlags(iconItem->flags() & ~Qt::ItemIsDropEnabled);
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
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        // if the click was down tree
        if (obj == ui->PrincipalWidget->viewport()) {
            if (!ui->PrincipalWidget->itemAt(mouseEvent->pos())) {
                ui->PrincipalWidget->clearSelection();
                ui->PrincipalWidget->setCurrentItem(nullptr);
                ui->metaDataTreeWidget->clear();
            }
        }
        // if the click was down icons
        else if (obj == ui->iconsWidget->viewport()) {
            if (!ui->iconsWidget->itemAt(mouseEvent->pos())) {
                ui->iconsWidget->clearSelection();
                ui->metaDataTreeWidget->clear();
            }
        }
    }

    if (event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent*>(event);

        if (obj == ui->PrincipalWidget->viewport()) {
            QTreeWidgetItem* itemDropped = ui->PrincipalWidget->currentItem();
            QTreeWidgetItem* targetItem = ui->PrincipalWidget->itemAt(dropEvent->position().toPoint());

            if (itemDropped) {
                Node* sourceNode = (Node*)itemDropped->data(0, Qt::UserRole).value<void*>();
                // if we drop in an item
                Node* destNode = targetItem ? (Node*)targetItem->data(0, Qt::UserRole).value<void*>() : currentFolder;

                if (sourceNode && destNode && destNode->isFolder) {
                    moveNodeLogic(sourceNode, destNode);
                    loadFolder(currentFolder, false);
                    return true;
                }
            }
        }
    }

    if (event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent*>(event);

        if (obj == ui->iconsWidget->viewport()) {
            QListWidgetItem* itemDragged = ui->iconsWidget->currentItem();
            QListWidgetItem* targetItem = ui->iconsWidget->itemAt(dropEvent->position().toPoint());

            if (itemDragged) {
                Node* sourceNode = (Node*)itemDragged->data(Qt::UserRole).value<void*>();
                Node* destNode = nullptr;

                if (targetItem) {
                    destNode = (Node*)targetItem->data(Qt::UserRole).value<void*>();
                }

                // only move if the destination is a folder
                if (sourceNode && destNode && destNode->isFolder) {
                    moveNodeLogic(sourceNode, destNode);
                    loadFolder(currentFolder, false);
                    return true;
                } else {
                    loadFolder(currentFolder, false);
                    return false;
                }
            }
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
        QMessageBox::warning(this, "Error", "You cant move or copy a folder inside of the same folder.");
        return;
    }

    string inputName = nodeToCopy->name;
    string finalName = inputName;

    if (isCutOperation) {
        moveNodeLogic(nodeToCopy, destination);
        nodeToCopy = nullptr;
        isCutOperation = false;
    } else {
        // copy logic yeah

        // check if the name already exists in tje dest
        if (manager.findChild(destination, inputName) != nullptr) {
            string baseName = inputName;
            string extension = "";

            // separate for get into yeah
            if (!nodeToCopy->isFolder) {
                size_t lastDot = inputName.find_last_of(".");
                if (lastDot != string::npos) {
                    baseName = inputName.substr(0, lastDot);
                    extension = inputName.substr(lastDot);
                }
            }

            int counter = 1;
            // search and rename the item
            while (manager.findChild(destination, baseName + " copy " + std::to_string(counter) + extension) != nullptr) {
                counter++;
            }
            finalName = baseName + " copy " + std::to_string(counter) + extension;
        }

        // copy in the memory
        Node* pastedNode = manager.copyNode(nodeToCopy, destination);
        pastedNode->name = finalName;
        pastedNode->creationDate = std::time(nullptr);
        pastedNode->modificationDate = std::time(nullptr);
        destination->modificationDate = std::time(nullptr);

        this->statusBar()->showMessage("Pasted: " + QString::fromStdString(finalName), 2000);
    }

    manager.saveBinary("System777.bin");
    loadFolder(currentFolder, false);
}

void MainWindow::copyAction() {
    Node* selectedNode = nullptr;

    if (ui->PrincipalWidget->isVisible()) {
        QTreeWidgetItem* item = ui->PrincipalWidget->currentItem();
        if (item) selectedNode = (Node*)item->data(0, Qt::UserRole).value<void*>();
    } else {
        QListWidgetItem* item = ui->iconsWidget->currentItem();
        if (item) selectedNode = (Node*)item->data(Qt::UserRole).value<void*>();
    }

    if (selectedNode) {
        nodeToCopy = selectedNode;
        nameAtCopyTime = selectedNode->name;
        isCutOperation = false;
        this->statusBar()->showMessage("Copied: " + QString::fromStdString(nodeToCopy->name), 2000);
    }
}

void MainWindow::cutAction() {
    Node* selectedNode = nullptr;

    if (ui->PrincipalWidget->isVisible()) {
        QTreeWidgetItem* item = ui->PrincipalWidget->currentItem();
        if (item) selectedNode = (Node*)item->data(0, Qt::UserRole).value<void*>();
    } else {
        QListWidgetItem* item = ui->iconsWidget->currentItem();
        if (item) selectedNode = (Node*)item->data(Qt::UserRole).value<void*>();
    }

    if (selectedNode) {
        nodeToCopy = selectedNode;
        isCutOperation = true;
        this->statusBar()->showMessage("Cut: " + QString::fromStdString(nodeToCopy->name), 2000);
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

    // dates in secs
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

void MainWindow::applyRestoreLogic(Node* nodeToRestore) {
    if (!nodeToRestore || !nodeToRestore->originalParent) return;

    Node* trash = manager.findChild(manager.root, ".trash");
    Node* dest = nodeToRestore->originalParent;
    string finalName = nodeToRestore->name;

    // anti-duplicates yeah
    if (manager.findChild(dest, finalName) != nullptr) {
        string base = finalName;
        string ext = "";
        if (!nodeToRestore->isFolder) {
            size_t dot = finalName.find_last_of(".");
            if (dot != string::npos) {
                base = finalName.substr(0, dot);
                ext = finalName.substr(dot);
            }
        }

        int counter = 1;
        while (manager.findChild(dest, base + " copy " + std::to_string(counter) + ext) != nullptr) {
            counter++;
        }
        finalName = base + " copy " + std::to_string(counter) + ext;
    }

    // move and update
    nodeToRestore->name = finalName;
    // bye bye on recycle bin
    trash->children.erase(std::remove(trash->children.begin(), trash->children.end(), nodeToRestore), trash->children.end());

    // back to original father
    nodeToRestore->parent = dest;
    dest->children.push_back(nodeToRestore);

    if (nodeToRestore->isFavorite) {
        // if isnt in the list we add mmmm
        if (!favoriteNodes.contains(nodeToRestore)) {
            favoriteNodes.append(nodeToRestore);
        }
    }

    // update dates
    nodeToRestore->modificationDate = std::time(nullptr);
    dest->modificationDate = std::time(nullptr);
    updateFavoritesUI();
}

void MainWindow::moveNodeLogic(Node* source, Node* destination) {
    if (!source || !destination || source == destination) return;

    // evade ancestors???/
    if (isAncestor(source, destination)) {
        QMessageBox::warning(this, "Error", "You cannot move a folder within itself.");
        return;
    }

    string finalName = source->name;
    Node* duplicate = manager.findChild(destination, finalName);

    if (duplicate != nullptr && duplicate != source) {
        int counter = 1;
        string base = finalName;
        string ext = "";

        if (!source->isFolder) {
            size_t dot = finalName.find_last_of(".");
            if (dot != string::npos) {
                base = finalName.substr(0, dot);
                ext = finalName.substr(dot);
            }
        }

        // search names for duplicates
        while (manager.findChild(destination, base + " " + std::to_string(counter) + ext) != nullptr) {
            counter++;
        }

        finalName = base + " " + std::to_string(counter) + ext;
    }

    // only move if the father changes
    if (source->parent != destination) {
        if (source->parent) {
            auto& v = source->parent->children;
            v.erase(std::remove(v.begin(), v.end(), source), v.end());
            source->parent->modificationDate = std::time(nullptr);
        }
        source->parent = destination;
        destination->children.push_back(source);
    }

    source->name = finalName;
    source->modificationDate = std::time(nullptr);
    destination->modificationDate = std::time(nullptr);
    manager.saveBinary("System777.bin");
}

void MainWindow::PrincipalWidget_customContextMenuRequested(const QPoint &pos)
{
    Node* selectedNode = nullptr;
    bool itemClicked = false;

    if (ui->PrincipalWidget->isVisible() && ui->PrincipalWidget->itemAt(pos)) {
        selectedNode = (Node*)ui->PrincipalWidget->itemAt(pos)->data(0, Qt::UserRole).value<void*>();
        itemClicked = true;
    } else if (ui->iconsWidget->isVisible() && ui->iconsWidget->itemAt(pos)) {
        selectedNode = (Node*)ui->iconsWidget->itemAt(pos)->data(Qt::UserRole).value<void*>();
        itemClicked = true;
    }

    QMenu menu(this);
    Node* trash = manager.findChild(manager.root, ".trash");
    bool inTrash = (currentFolder == trash);

    if (inTrash) {
        // recyble bin
        if (itemClicked && selectedNode) {
            QAction* restoreAct = menu.addAction("♻️ Restore");
            restoreAct->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::SystemReboot));
            connect(restoreAct, &QAction::triggered, this, [=]() {
                applyRestoreLogic(selectedNode);
                manager.saveBinary("System777.bin");
                loadFolder(trash);
                ui->metaDataTreeWidget->clear();
                this->statusBar()->showMessage("Restored: " + QString::fromStdString(selectedNode->name), 2000);
            });

            QAction* deletePermAct = menu.addAction("❌ Delete Permanently");
            connect(deletePermAct, &QAction::triggered, this, [=]() {
                if (QMessageBox::question(this, "Permanent Delete", "This cannot be undone. Delete?") == QMessageBox::Yes) {
                    ui->metaDataTreeWidget->clear();
                    manager.deleteNode(selectedNode);
                    manager.saveBinary("System777.bin");
                    loadFolder(trash);
                }
            });
        } else {
            // click in the blank space
            QAction* restoreAllAct = menu.addAction("♻️ Restore All");
            connect(restoreAllAct, &QAction::triggered, this, [=]() {
                while (!trash->children.empty()) {
                    applyRestoreLogic(trash->children[0]);
                }
                manager.saveBinary("System777.bin");
                loadFolder(trash);
                ui->metaDataTreeWidget->clear();
                this->statusBar()->showMessage("All items restored successfully", 2000);
            });

            QAction* emptyTrashAct = menu.addAction("❌ Empty Trash");
            connect(emptyTrashAct, &QAction::triggered, this, [=]() {
                if (QMessageBox::warning(this, "Empty Trash", "Delete all items?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    while (!trash->children.empty()) {
                        manager.deleteNode(trash->children[0]);
                    }
                    manager.saveBinary("System777.bin");
                    loadFolder(trash);
                    ui->metaDataTreeWidget->clear();
                }
            });
        }
    } else {
        if (itemClicked && selectedNode) {
            // click under a folder or file
            if (selectedNode->isFolder) {
                QAction* openAct = menu.addAction("Open Folder");
                openAct->setIcon(this->style()->standardIcon(QStyle::SP_DirOpenIcon));
                connect(openAct, &QAction::triggered, this, [=]() { loadFolder(selectedNode); });
            } else {
                QAction* openNotepad = menu.addAction("📝 Open with Notepad");
                connect(openNotepad, &QAction::triggered, this, [=]() {
                    Notepad *notepad = new Notepad(selectedNode, &manager);
                    connect(notepad, &Notepad::fileSaved, this, &MainWindow::updateMetadata);
                    notepad->setAttribute(Qt::WA_DeleteOnClose);
                    notepad->show();
                });
            }
            menu.addSeparator();

            QAction* cutAct = menu.addAction("✂️ Cut");
            cutAct->setShortcut(QKeySequence::Cut);
            connect(cutAct, &QAction::triggered, this, &MainWindow::cutAction);

            QAction* copyAct = menu.addAction("📑 Copy");
            copyAct->setShortcut(QKeySequence::Copy);
            connect(copyAct, &QAction::triggered, this, &MainWindow::copyAction);

            if (nodeToCopy && selectedNode->isFolder) {
                QAction* pasteInAct = menu.addAction("📋 Paste inside '" + QString::fromStdString(selectedNode->name) + "'");
                connect(pasteInAct, &QAction::triggered, this, [=]() { pasteLogic(selectedNode); updateMetadata(selectedNode);});
            }
            menu.addSeparator();

            // rename
            QAction* renameAct = menu.addAction("✏️ Rename");
            connect(renameAct, &QAction::triggered, this, [=]() {
                bool ok;
                std::string nameToEdit = selectedNode->name;
                if (!selectedNode->isFolder) {
                    size_t lastDot = nameToEdit.find_last_of(".");
                    if (lastDot != std::string::npos) nameToEdit = nameToEdit.substr(0, lastDot);
                }

                QString inputName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal,
                                                          QString::fromStdString(nameToEdit), &ok).trimmed();

                if (ok) {
                    // new security validation [help me pls]
                    string illegal = "*?<>|/\\:\"";
                    bool hasIllegal = false;
                    for(char c : illegal) if(inputName.contains(c)) hasIllegal = true;

                    if (inputName.isEmpty() || inputName == "." || inputName == ".." || hasIllegal) {
                        QMessageBox::critical(this, "Error", "Invalid name! Dont use special characters.");
                        return;
                    }

                    std::string newName = inputName.toStdString();
                    // extensions case
                    if (selectedNode->isFolder) {
                        if (newName.length() >= 4 && newName.substr(newName.length() - 4) == ".txt")
                            newName = newName.substr(0, newName.length() - 4);
                    } else {
                        if (newName.length() < 4 || newName.substr(newName.length() - 4) != ".txt")
                            newName += ".txt";
                    }

                    Node* duplicate = manager.findChild(currentFolder, newName);
                    if (duplicate != nullptr && duplicate != selectedNode) {
                        QMessageBox::warning(this, "Error", "There is already an element with this name!");
                    } else {
                        manager.renameNode(selectedNode, newName);

                        time_t now = std::time(nullptr);
                        selectedNode->modificationDate = now;

                        if (selectedNode->parent) {
                            selectedNode->parent->modificationDate = now;
                        }
                        manager.saveBinary("System777.bin");
                        loadFolder(currentFolder, false);
                        updateFavoritesUI();
                        updateMetadata(selectedNode);
                    }
                }
            });

            // delete
            QAction* deleteAct = menu.addAction("🗑️ Delete");
            connect(deleteAct, &QAction::triggered, this, [=]() {
                Node* trash = manager.findChild(manager.root, ".trash");
                if (!selectedNode || selectedNode == manager.root || selectedNode == trash) return;

                if (selectedNode == nodeToCopy) {
                    nodeToCopy = nullptr;
                    this->statusBar()->showMessage("Copy operation cancelled! [source deleted]", 2000);
                }

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
                    selectedNode->isFavorite = true;
                    favoriteNodes.append(selectedNode);
                    updateFavoritesUI();
                    manager.saveBinary("System777.bin");
                    this->statusBar()->showMessage("Added to favorites", 2000);
                } else {
                    QMessageBox::critical(this, "Favorites", "'" + QString::fromStdString(selectedNode->name) + "' is already in your favorites!");
                }
            });

        } else {
            // click in a blank space

            QMenu* viewMenu = menu.addMenu("👀 View in");

            QAction* viewListAct = viewMenu->addAction("📄 List");
            QAction* viewIconsAct = viewMenu->addAction("🖼️ Icons");

            connect(viewListAct, &QAction::triggered, this, [=]() {
                ui->iconsWidget->hide();
                ui->PrincipalWidget->show();
                manager.isIconMode = false;
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder);
            });

            connect(viewIconsAct, &QAction::triggered, this, [=]() {
                ui->PrincipalWidget->hide();
                ui->iconsWidget->show();
                manager.isIconMode = true;
                manager.saveBinary("System777.bin");
                loadFolder(currentFolder);
            });

            menu.addSeparator();

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
                QString name = QInputDialog::getText(this, "New File", "Name:", QLineEdit::Normal, "", &ok).trimmed();

                if (ok) {
                    string illegal = "*?<>|/\\:\"";
                    bool hasIllegal = false;
                    for(char c : illegal) if(name.contains(c)) hasIllegal = true;

                    if (name.isEmpty() || name == "." || name == ".." || hasIllegal) {
                        QMessageBox::critical(this, "Error", "Invalid name! Dont use special characters.");
                        return;
                    }

                    string finalName = name.toStdString();
                    if (finalName.length() < 4 || finalName.substr(finalName.length() - 4) != ".txt")
                        finalName += ".txt";

                    string baseName = finalName.substr(0, finalName.length() - 4);
                    int counter = 1;
                    while (manager.findChild(currentFolder, finalName) != nullptr) {
                        finalName = baseName + " " + std::to_string(counter++) + ".txt";
                    }

                    manager.addNode(currentFolder, finalName, false);
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
                QString name = QInputDialog::getText(this, "New Folder", "Name:", QLineEdit::Normal, "", &ok).trimmed();

                if (ok) {
                    string illegal = "*?<>|/\\:\"";
                    bool hasIllegal = false;
                    for(char c : illegal) if(name.contains(c)) hasIllegal = true;

                    if (name.isEmpty() || name == "." || name == ".." || hasIllegal) {
                        QMessageBox::critical(this, "Error", "Invalid name! Dont use special characters.");
                        return;
                    }

                    string finalName = name.toStdString();
                    if (finalName.length() >= 4 && finalName.substr(finalName.length() - 4) == ".txt")
                        finalName = finalName.substr(0, finalName.length() - 4);

                    string baseName = finalName;
                    int counter = 1;
                    while (manager.findChild(currentFolder, finalName) != nullptr) {
                        finalName = baseName + " " + std::to_string(counter++);
                    }

                    manager.addNode(currentFolder, finalName, true);
                    manager.saveBinary("System777.bin");
                    loadFolder(currentFolder, false);
                    updateFavoritesUI();
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

