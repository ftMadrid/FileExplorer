#include "node.h"

#include <string.h>

using std::string;

Node::Node(string name, bool isFolder) {
    this->name = name;
    this->isFolder = isFolder;
    this->content = "";
    this->left = nullptr;
    this->right = nullptr;
}
