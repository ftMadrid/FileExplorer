#ifndef HISTORYLIST_H
#define HISTORYLIST_H

#include "node.h"

struct HistoryNode {
    Node* folder;
    HistoryNode* prev;
    HistoryNode* next;

    HistoryNode(Node* n) : folder(n), prev(nullptr), next(nullptr) {}
};

class HistoryList {
private:
    void deleteForward(HistoryNode* node); // cleanup

public:
    HistoryNode* current;

    HistoryList();
    ~HistoryList();

    void addStep(Node* n);
};

#endif // HISTORYLIST_H
