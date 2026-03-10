#include "node.h"

Node::Node(string name, bool isFolder, Node* parent) {
    this->name = name;
    this->isFolder = isFolder;
    this->content = "";
    this->isFavorite = false;
    this->parent = parent;
}

Node::~Node() {
    for (Node* child : children) {
        delete child;
    }
    children.clear();
}
