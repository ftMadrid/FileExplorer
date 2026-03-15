#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "filemanager.h"
#include <QTreeWidgetItem>
#include "historylist.h"
#include <QStandardItemModel>
#include <QList>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_PrincipalWidget_customContextMenuRequested(const QPoint &pos);
    void on_favoritesTreeView_customContextMenuRequested(const QPoint &pos);
    void on_backButton_clicked();
    void on_nextButton_clicked();
    void on_fatherButton_clicked();
    void on_searchButton_clicked();

private:
    Ui::MainWindow *ui;
    FileManager manager;
    Node* nodeToCopy = nullptr;
    void refreshUI();
    void drawTree(Node* node, QTreeWidgetItem* visualParent); // recursive draw
    Node* currentFolder;
    HistoryList history;
    void loadFolder(Node* folder, bool addToHistory = true);
    QString getPath(Node* node);
    bool isAncestor(Node* potentialAncestor, Node* target);
    void pasteLogic(Node* destination);
    void copyAction();
    QStandardItemModel *favoritesModel;
    QStandardItemModel *metaDataModel;
    QList<Node*> favoriteNodes;
    void updateFavoritesUI();
    void collectFavorites(Node* node);
    void updateMetadata(Node* node);
    long calculateTotalSize(Node* node);
    QString formatSize(long bytes);
    bool isCutOperation = false;
    void cutAction();
    void applyRestoreLogic(Node* nodeToRestore);

protected:
    // func just to deselected an item when we clicked in another place
    bool eventFilter(QObject *obj, QEvent *event) override;

};

#endif // MAINWINDOW_H
