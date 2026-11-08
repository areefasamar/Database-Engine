#include "avl_index.h"

using namespace std;

AVLIndex::AVLIndex() {
    root = nullptr;
    count = 0;
}

size_t AVLIndex::Size() const {
    return count;
}

bool AVLIndex::Get(Key key, string* out) const {
    Node* current = root;

    while (current != nullptr) {
        if (key == current->key) {
            if (out != nullptr) {
                *out = current->value;
            }
            return true;
        }

        if (key < current->key) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }

    return false;
}

void AVLIndex::clearTree(Node* node) {
    if (node == nullptr) {
        return;
    }

    clearTree(node->left);
    clearTree(node->right);
    delete node;
}

AVLIndex::~AVLIndex() {
    clearTree(root);
}

AVLIndex::Node* AVLIndex::insertNode(
    Node* node, Key key, const string& value, bool& inserted) {

    if (node == nullptr) {
        Node* newNode = new Node(key, value);
        inserted = true;
        return newNode;
    }

    if (key < node->key) {
        node->left = insertNode(node->left, key, value, inserted);
    }
    else if (key > node->key) {
        node->right = insertNode(node->right, key, value, inserted);
    }
    else {
        node->value = value;
    }

    return node;
}

bool AVLIndex::Put(Key key, const string& value) {
    bool inserted = false;
    root = insertNode(root, key, value, inserted);

    if (inserted) {
        ++count;
    }

    return inserted;
}