#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <ctime>

using std::string;
using std::vector;
using std::time_t;

class Node {
public:
    string name;
    bool isFolder;
    string content;
    bool isFavorite;
    Node* parent;
    vector<Node*> children;
    Node* originalParent = nullptr;

    time_t creationDate;
    time_t modificationDate;

    Node(string name, bool isFolder, Node* parent = nullptr);
    ~Node();
};

#endif
