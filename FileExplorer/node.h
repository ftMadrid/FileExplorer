#ifndef NODE_H
#define NODE_H

#include <string>

using std::string;

class Node {
public:
    string name;
    bool isFolder;
    string content;
    Node* left;
    Node* right;
    Node* parent;

    Node(string name, bool isFolder, Node* parent = nullptr);
};

#endif
