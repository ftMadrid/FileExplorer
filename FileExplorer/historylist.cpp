#include "historylist.h"

HistoryList::HistoryList() {
    current = nullptr;
}

// cleans up the entire list from the current position (in simply words is the destructor omg)
HistoryList::~HistoryList() {
    if (current) {
        // go to the very beginning of the history
        HistoryNode* tmp = current;
        while (tmp->prev) {
            tmp = tmp->prev;
        }
        deleteForward(tmp);
    }
}

void HistoryList::addStep(Node* n) {
    if (!n) return;

    // we delete the forward branch (like a browser)
    if (current && current->next) {
        deleteForward(current->next);
        current->next = nullptr;
    }

    HistoryNode* newNode = new HistoryNode(n);

    if (current) {
        current->next = newNode;
        newNode->prev = current;
    }

    current = newNode;
}

// recursive func to clear memory when branching or deleting
void HistoryList::deleteForward(HistoryNode* node) {
    if (!node) return;
    deleteForward(node->next);
    delete node;
}
