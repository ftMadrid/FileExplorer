#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>

using std::string;
using std::vector;

class Node {
public:
    string name;
    bool isFolder;
    string content;
    bool isFavorite;
    Node* parent;
    vector<Node*> children;

    Node(string name, bool isFolder, Node* parent = nullptr);
    ~Node();
};

#endif
