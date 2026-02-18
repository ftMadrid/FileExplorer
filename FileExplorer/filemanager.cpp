#include "filemanager.h"

FileManager::FileManager() {
    root = new Node("C:/", true);
}

FileManager::~FileManager() {
    deleteTree(root);
}

void FileManager::deleteTree(Node* node) {
    if (!node) return;
    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}

void FileManager::addNode(Node* parent, string name, bool isFolder) {
    if (!parent) return;
    Node* newNode = new Node(name, isFolder);
    if (!parent->left) {
        parent->left = newNode;
    } else {
        Node* temp = parent->left;
        while (temp->right) {
            temp = temp->right;
        }
        temp->right = newNode;
    }
}

void FileManager::saveBinary(string filename) {
    ofstream out(filename, std::ios::binary);
    if (out.is_open()) {
        saveTree(root, out);
        out.close();
    }
}

void FileManager::saveTree(Node* node, ofstream& out) {
    char isNull = (node == nullptr) ? 1 : 0;
    out.write(&isNull, sizeof(char));
    if (!node) return;

    int nameSize = (int)node->name.size();
    out.write((char*)&nameSize, sizeof(int));
    out.write(node->name.c_str(), nameSize);

    char folderFlag = node->isFolder ? 1 : 0;
    out.write(&folderFlag, sizeof(char));

    int contentSize = (int)node->content.size();
    out.write((char*)&contentSize, sizeof(int));
    out.write(node->content.c_str(), contentSize);

    saveTree(node->left, out);
    saveTree(node->right, out);
}

void FileManager::loadBinary(string filename) {
    ifstream in(filename, std::ios::binary);
    if (in.is_open()) {
        Node* tempRoot = loadTree(in);
        if (tempRoot) {
            deleteTree(root);
            root = tempRoot;
        }
        in.close();
    }
}

Node* FileManager::loadTree(ifstream& in) {
    char isNull;
    if (!in.read(&isNull, sizeof(char)) || isNull == 1) return nullptr;

    int nameSize = 0;
    in.read((char*)&nameSize, sizeof(int));
    if (nameSize < 0 || nameSize > 1000) return nullptr;

    char* buffer = new char[nameSize + 1];
    in.read(buffer, nameSize);
    buffer[nameSize] = '\0';
    string name(buffer);
    delete[] buffer;

    char folderFlag;
    in.read(&folderFlag, sizeof(char));
    bool isFolder = (folderFlag == 1);

    Node* newNode = new Node(name, isFolder);

    int contentSize = 0;
    in.read((char*)&contentSize, sizeof(int));
    if (contentSize >= 0 && contentSize < 1000000) {
        char* cBuffer = new char[contentSize + 1];
        in.read(cBuffer, contentSize);
        cBuffer[contentSize] = '\0';
        newNode->content = string(cBuffer);
        delete[] cBuffer;
    }

    newNode->left = loadTree(in);
    newNode->right = loadTree(in);
    return newNode;
}
