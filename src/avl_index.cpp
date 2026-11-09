#include <algorithm>
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

    return rebalance(node);
}

bool AVLIndex::Put(Key key, const string& value) {
    bool inserted = false;
    root = insertNode(root, key, value, inserted);

    if (inserted) {
        ++count;
    }

    return inserted;
}

int AVLIndex::nodeHeight(Node* node) const {
    if (node == nullptr) {
        return 0;
    }
    return node->height;
}

void AVLIndex::updateHeight(Node* node) {
    node->height = 1 + max(nodeHeight(node->left),
                           nodeHeight(node->right));
}

int AVLIndex::Height() const {
    return nodeHeight(root);
}

int AVLIndex::balanceFactor(Node* node) const {
    if (node == nullptr) {
        return 0;
    }

    return nodeHeight(node->left) - nodeHeight(node->right);
}

AVLIndex::Node* AVLIndex::rotateRight(Node* node) {
    Node* newRoot = node->left;
    Node* transferred = newRoot->right;

    newRoot->right = node;
    node->left = transferred;

    updateHeight(node);
    updateHeight(newRoot);

    return newRoot;
}

AVLIndex::Node* AVLIndex::rotateLeft(Node* node) {
    Node* newRoot = node->right;
    Node* transferred = newRoot->left;

    newRoot->left = node;
    node->right = transferred;

    updateHeight(node);
    updateHeight(newRoot);

    return newRoot;
}

AVLIndex::Node* AVLIndex::rebalance(Node* node) {
    if (node == nullptr) {
        return nullptr;
    }

    updateHeight(node);
    int balance = balanceFactor(node);

    if (balance > 1) {
        if (balanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left); // LR case
        }
        return rotateRight(node); // LL, or second step of LR
    }

    if (balance < -1) {
        if (balanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right); // RL case
        }
        return rotateLeft(node); // RR, or second step of RL
    }

    return node;
}
