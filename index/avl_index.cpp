#include "avl_index.h"
#include <algorithm>
#include <limits>

// ═══════════════════════════════════════════════════════ helpers ══════════════

int AVLIndex::height(Node* n) const {
    return n ? n->height : 0;
}

int AVLIndex::balance(Node* n) const {
    return n ? height(n->left) - height(n->right) : 0;
}

// ══════════════════════════════════════════════════════ rotations ═════════════

AVLIndex::Node* AVLIndex::rotateRight(Node* y) {
    Node* x  = y->left;
    Node* T2 = x->right;
    x->right = y;
    y->left  = T2;
    y->height = 1 + std::max(height(y->left), height(y->right));
    x->height = 1 + std::max(height(x->left), height(x->right));
    return x;
}

AVLIndex::Node* AVLIndex::rotateLeft(Node* x) {
    Node* y  = x->right;
    Node* T2 = y->left;
    y->left  = x;
    x->right = T2;
    x->height = 1 + std::max(height(x->left), height(x->right));
    y->height = 1 + std::max(height(y->left), height(y->right));
    return y;
}

// ═════════════════════════════════════════════════════════ helpers ════════════

AVLIndex::Node* AVLIndex::minNode(Node* n) const {
    while (n && n->left) n = n->left;
    return n;
}

void AVLIndex::destroyTree(Node* n) {
    if (!n) return;
    destroyTree(n->left);
    destroyTree(n->right);
    delete n;
}

// ══════════════════════════════════════════════════════════ insert ════════════

AVLIndex::Node* AVLIndex::insert(Node* n, Key key,
                                  const std::string& value, bool& isNew) {
    if (!n) {
        isNew = true;
        ++count;
        return new Node(key, value);
    }
    if      (key < n->key) n->left  = insert(n->left,  key, value, isNew);
    else if (key > n->key) n->right = insert(n->right, key, value, isNew);
    else {
        n->value = value;   // update existing key
        isNew = false;
        return n;
    }

    n->height = 1 + std::max(height(n->left), height(n->right));
    int b = balance(n);

    // Left-Left
    if (b > 1 && key < n->left->key)
        return rotateRight(n);
    // Right-Right
    if (b < -1 && key > n->right->key)
        return rotateLeft(n);
    // Left-Right
    if (b > 1 && key > n->left->key) {
        n->left = rotateLeft(n->left);
        return rotateRight(n);
    }
    // Right-Left
    if (b < -1 && key < n->right->key) {
        n->right = rotateRight(n->right);
        return rotateLeft(n);
    }
    return n;
}

// ══════════════════════════════════════════════════════════ remove ════════════

AVLIndex::Node* AVLIndex::removeNode(Node* n, Key key, bool& removed) {
    if (!n) return nullptr;

    if      (key < n->key) n->left  = removeNode(n->left,  key, removed);
    else if (key > n->key) n->right = removeNode(n->right, key, removed);
    else {
        // Found the node to delete
        removed = true;
        --count;

        if (!n->left || !n->right) {
            // 0 or 1 child — simple splice-out
            Node* child = n->left ? n->left : n->right;
            delete n;
            return child;
        }

        // 2 children: replace with in-order successor
        Node* successor = minNode(n->right);
        n->key   = successor->key;
        n->value = successor->value;
        // Now delete the successor from the right subtree.
        // We pass a dummy bool so count isn't decremented twice.
        bool dummy = false;
        ++count;    // temporarily restore count before the recursive delete
        n->right = removeNode(n->right, successor->key, dummy);
    }

    if (!n) return nullptr;    // shouldn't happen, but be safe

    n->height = 1 + std::max(height(n->left), height(n->right));
    int b = balance(n);

    // Left-Left
    if (b > 1 && balance(n->left) >= 0)
        return rotateRight(n);
    // Left-Right
    if (b > 1 && balance(n->left) < 0) {
        n->left = rotateLeft(n->left);
        return rotateRight(n);
    }
    // Right-Right
    if (b < -1 && balance(n->right) <= 0)
        return rotateLeft(n);
    // Right-Left
    if (b < -1 && balance(n->right) > 0) {
        n->right = rotateRight(n->right);
        return rotateLeft(n);
    }
    return n;
}

// ═══════════════════════════════════════════════════════════ range ════════════

void AVLIndex::rangeQuery(Node* n, Key lo, Key hi,
                           std::vector<KeyValue>& out) const {
    if (!n) return;
    if (lo < n->key)                    rangeQuery(n->left,  lo, hi, out);
    if (lo <= n->key && n->key <= hi)   out.push_back({n->key, n->value});
    if (hi > n->key)                    rangeQuery(n->right, lo, hi, out);
}

// ══════════════════════════════════════════════════════ validation ════════════

bool AVLIndex::checkValid(Node* n,
                           Key minKey, Key maxKey,
                           bool hasMin, bool hasMax) const {
    if (!n) return true;

    // BST ordering
    if (hasMin && n->key <= minKey) return false;
    if (hasMax && n->key >= maxKey) return false;

    // AVL balance factor must be in {-1, 0, 1}
    int b = balance(n);
    if (b < -1 || b > 1) return false;

    // Stored height must be correct
    int expected = 1 + std::max(height(n->left), height(n->right));
    if (n->height != expected) return false;

    return checkValid(n->left,  minKey, n->key, hasMin, true)
        && checkValid(n->right, n->key, maxKey, true,   hasMax);
}

bool AVLIndex::IsValid() const {
    return checkValid(root, 0, 0, false, false);
}

// ══════════════════════════════════════════════════════ public API ════════════

AVLIndex::AVLIndex() : root(nullptr), count(0) {}
AVLIndex::~AVLIndex() { destroyTree(root); }

bool AVLIndex::Put(Key key, const std::string& value) {
    bool isNew = false;
    root = insert(root, key, value, isNew);
    return isNew;
}

bool AVLIndex::Get(Key key, std::string* out) const {
    Node* n = root;
    while (n) {
        if      (key < n->key) n = n->left;
        else if (key > n->key) n = n->right;
        else { if (out) *out = n->value; return true; }
    }
    return false;
}

bool AVLIndex::Remove(Key key) {
    bool removed = false;
    root = removeNode(root, key, removed);
    return removed;
}

std::vector<KeyValue> AVLIndex::Range(Key lo, Key hi) const {
    std::vector<KeyValue> out;
    rangeQuery(root, lo, hi, out);
    return out;
}

std::size_t AVLIndex::Size() const { return count; }

int AVLIndex::Height() const { return height(root); }
