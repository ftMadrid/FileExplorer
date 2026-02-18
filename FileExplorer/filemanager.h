#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "node.h"
#include <fstream>
#include <iostream>

using std::ofstream;
using std::ifstream;
using std::string;

class FileManager {
private:
    Node* loadTree(std::ifstream& in, Node* parentNode);
    void saveTree(Node* node, std::ofstream& out);
    void deleteTree(Node* node);

public:
    Node* root;

    FileManager();
    ~FileManager();

    void addNode(Node* parent, string name, bool isFolder);
    void deleteNode(Node* target);
    void renameNode(Node* target, string newName);
    void saveBinary(string filename);
    void loadBinary(string filename);
    Node* searchNode(Node* current, string name);
    Node* findChild(Node* parent, string name);
};

#endif
