#include "filemanager.h"

FileManager::FileManager() {
    root = new Node("C:/", true);

    if (!findChild(root, ".trash")) {
        addNode(root, ".trash", true);
    }
}

FileManager::~FileManager() {
    delete root;
}

void FileManager::addNode(Node* parent, string name, bool isFolder) {
    if (!parent || !parent->isFolder) return;
    if (findChild(parent, name)) return; // for duplicates

    Node* newNode = new Node(name, isFolder, parent);
    parent->children.push_back(newNode);
}

void FileManager::renameNode(Node* target, string newName) {
    if (!target) return;
    target->name = newName;
}

void FileManager::deleteNode(Node* target) {
    if (!target || target == root) return;
    Node* p = target->parent;
    if (!p) return;

    // search and delete vector from the father
    for (auto it = p->children.begin(); it != p->children.end(); ++it) {
        if (*it == target) {
            delete *it;
            p->children.erase(it);
            break;
        }
    }
}

// --- persistent ---

void FileManager::saveBinary(string filename) {
    ofstream out(filename, std::ios::binary);
    if (out.is_open()) {
        saveTree(root, out);
        out.close();
    }
}

void FileManager::saveTree(Node* node, ofstream& out) {
    if (!node) return;

    // save name
    int nameSize = (int)node->name.size();
    out.write((char*)&nameSize, sizeof(int));
    out.write(node->name.c_str(), nameSize);

    // save if its folder
    char folderFlag = node->isFolder ? 1 : 0;
    out.write(&folderFlag, sizeof(char));

    // save content (only files)
    int contentSize = (int)node->content.size();
    out.write((char*)&contentSize, sizeof(int));
    out.write(node->content.c_str(), contentSize);

    char favFlag = node->isFavorite ? 1 : 0;
    out.write(&favFlag, sizeof(char));

    out.write((char*)&node->creationDate, sizeof(std::time_t));
    out.write((char*)&node->modificationDate, sizeof(std::time_t));

    // save the amount of childs
    int childCount = (int)node->children.size();
    out.write((char*)&childCount, sizeof(int));

    for (Node* child : node->children) {
        saveTree(child, out);
    }
}

void FileManager::loadBinary(string filename) {
    ifstream in(filename, std::ios::binary);
    if (in.is_open()) {
        Node* tempRoot = loadTree(in, nullptr);
        if (tempRoot) {
            delete root;
            root = tempRoot;
        }
        in.close();
    }
}

Node* FileManager::loadTree(ifstream& in, Node* parentNode) {
    int nameSize = 0;
    if (!in.read((char*)&nameSize, sizeof(int))) return nullptr;

    char* buffer = new char[nameSize + 1];
    in.read(buffer, nameSize);
    buffer[nameSize] = '\0';
    string name(buffer);
    delete[] buffer;

    char folderFlag;
    in.read(&folderFlag, sizeof(char));

    Node* newNode = new Node(name, folderFlag == 1, parentNode);

    int contentSize = 0;
    in.read((char*)&contentSize, sizeof(int));
    if (contentSize > 0) {
        char* cBuffer = new char[contentSize + 1];
        in.read(cBuffer, contentSize);
        cBuffer[contentSize] = '\0';
        newNode->content = string(cBuffer);
        delete[] cBuffer;
    }

    char favFlag;
    in.read(&favFlag, sizeof(char));
    newNode->isFavorite = (favFlag == 1); // load the state

    in.read((char*)&newNode->creationDate, sizeof(std::time_t));
    in.read((char*)&newNode->modificationDate, sizeof(std::time_t));

    // read amount of childs
    int childCount = 0;
    in.read((char*)&childCount, sizeof(int));

    for (int i = 0; i < childCount; i++) {
        Node* child = loadTree(in, newNode);
        if (child) newNode->children.push_back(child);
    }

    return newNode;
}

Node* FileManager::findChild(Node* parent, string name) {
    if (!parent) return nullptr;
    for (Node* child : parent->children) {
        if (child->name == name) return child;
    }
    return nullptr;
}

Node* FileManager::searchNode(Node* current, string name) {
    if (!current) return nullptr;
    if (current->name == name) return current;

    for (Node* child : current->children) {
        Node* found = searchNode(child, name);
        if (found) return found;
    }
    return nullptr;
}

Node* FileManager::copyNode(Node* source, Node* newParent) {
    if (!source) return nullptr;

    Node* newNode = new Node(source->name, source->isFolder, newParent);
    newNode->content = source->content;

    // if we are copy from a father, we add into the list
    if (newParent) {
        newParent->children.push_back(newNode);
    }

    for (Node* child : source->children) {
        copyNode(child, newNode);
    }

    return newNode;
}
