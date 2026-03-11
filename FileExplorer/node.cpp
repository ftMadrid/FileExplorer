#include "node.h"

Node::Node(string name, bool isFolder, Node* parent) {
    this->name = name;
    this->isFolder = isFolder;
    this->content = "";
    this->isFavorite = false;
    this->parent = parent;
    this->creationDate = std::time(nullptr);
    this->modificationDate = std::time(nullptr);
}

Node::~Node() {
    for (Node* child : children) {
        delete child;
    }
    children.clear();
}
