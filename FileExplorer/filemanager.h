#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "node.h"
#include <fstream>
#include <string>

using std::string;
using std::ofstream;
using std::ifstream;

class FileManager {
private:
    Node* loadTree(ifstream& in, Node* parentNode);
    void saveTree(Node* node, ofstream& out);

public:
    Node* root;

    FileManager();
    ~FileManager();

    void loadBinary(string filename);
    void saveBinary(string filename);
    void renameNode(Node* target, string newName);

    void addNode(Node* parent, string name, bool isFolder);
    void deleteNode(Node* target);
    Node* findChild(Node* parent, string name);
    Node* searchNode(Node* current, string name);
    Node* copyNode(Node* source, Node* newParent);
    bool isIconMode = false;
    int iconSize = 64;
};

#endif
