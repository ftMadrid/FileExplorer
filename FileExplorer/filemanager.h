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
    void saveTree(Node* node, ofstream& out);
    Node* loadTree(ifstream& in);
    void deleteTree(Node* node);

public:
    Node* root;

    FileManager();
    ~FileManager();

    void addNode(Node* parent, string name, bool isFolder);
    void saveBinary(string filename);
    void loadBinary(string filename);
};

#endif
