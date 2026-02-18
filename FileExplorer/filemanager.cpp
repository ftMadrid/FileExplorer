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

void FileManager::addNode(Node* parentNode, string name, bool isFolder) {
    if (!parentNode) return;

    Node* newNode = new Node(name, isFolder, parentNode);

    if (!parentNode->left) {
        parentNode->left = newNode;
    } else {
        Node* tmp = parentNode->left;
        while (tmp->right) {
            tmp = tmp->right;
        }
        tmp->right = newNode;
    }
}

void FileManager::deleteNode(Node* target) {
    if (!target || target == root) return; // cant delete C:/ [i know you want to do it inge]

    Node* p = target->parent;
    if (!p) return;

    // lefts
    if (p->left == target) {
        p->left = target->right;
    }
    // rights
    else {
        Node* tmp = p->left;
        while (tmp && tmp->right != target) {
            tmp = tmp->right;
        }
        if (tmp) {
            tmp->right = target->right;
        }
    }

    // get out the node from the others to delete
    target->right = nullptr;
    deleteTree(target);
}

void FileManager::renameNode(Node* target, string newName) {
    if (!target) return;

    // check if is file (.txt)
    if (!target->isFolder) {
        if (newName.length() < 4 || newName.substr(newName.length() - 4) != ".txt") {
            newName += ".txt";
        }
    }
    target->name = newName;
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
        Node* tempRoot = loadTree(in, nullptr);
        if (tempRoot) {
            deleteTree(root);
            root = tempRoot;
        }
        in.close();
    }
}

Node* FileManager::loadTree(ifstream& in, Node* parentNode) {
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

    Node* newNode = new Node(name, isFolder, parentNode);

    int contentSize = 0;
    in.read((char*)&contentSize, sizeof(int));
    if (contentSize >= 0 && contentSize < 1000000) {
        char* cBuffer = new char[contentSize + 1];
        in.read(cBuffer, contentSize);
        cBuffer[contentSize] = '\0';
        newNode->content = string(cBuffer);
        delete[] cBuffer;
    }

    newNode->left = loadTree(in, newNode);
    newNode->right = loadTree(in, parentNode);

    return newNode;
}

Node* FileManager::findChild(Node* parent, string name) {
    if (!parent || !parent->left) return nullptr;

    // start at first child
    Node* tmp = parent->left;
    while (tmp) {
        if (tmp->name == name) {
            return tmp; // founded hehe
        }
        tmp = tmp->right; // brothers next
    }
    return nullptr;
}

Node* FileManager::searchNode(Node* current, string name) {
    if (!current) return nullptr;

    if (current->name == name) {
        return current;
    }

    // left search
    Node* found = searchNode(current->left, name);
    if (found) return found;

    // right search
    return searchNode(current->right, name);
}

Node* FileManager::copyNode(Node* source, Node* newParent) {
    if (!source) return nullptr;

    // create the actual clone
    Node* newNode = new Node(source->name, source->isFolder, newParent);
    newNode->content = source->content;

    // copy left
    newNode->left = copyNode(source->left, newNode);
    // copy right instead yeah yeah
    newNode->right = copyNode(source->right, newParent);

    return newNode;
}
