#ifndef MINIDB_AVL_INDEX_H
#define MINIDB_AVL_INDEX_H

// AVL-tree backed index.
// Self-balancing BST: O(log N) insert, lookup, and delete.
// Implementation by Member 2 (Index Layer team).

#include "../index.h"

class AVLIndex : public Index {
public:
    AVLIndex();
    ~AVLIndex() override;

    AVLIndex(const AVLIndex&) = delete;
    AVLIndex& operator=(const AVLIndex&) = delete;

    bool Put(Key key, const std::string& value) override;
    bool Get(Key key, std::string* out) const override;
    bool Remove(Key key) override;
    std::vector<KeyValue> Range(Key lo, Key hi) const override;
    std::size_t Size() const override;

    // Verifies BST ordering AND AVL balance invariants on every node.
    // Returns true when the tree is well-formed. Used by tests.
    bool IsValid() const;

    // Returns the total height of the tree (0 if empty).
    int Height() const;

private:
    struct Node {
        Key key;
        std::string value;
        Node* left;
        Node* right;
        int height;
        Node(Key k, const std::string& v)
            : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
    };

    Node* root;
    std::size_t count;

    // ── internal helpers ──────────────────────────────────────────────
    int    height(Node* n) const;
    int    balance(Node* n) const;
    Node*  rotateRight(Node* y);
    Node*  rotateLeft(Node* x);
    Node*  insert(Node* n, Key key, const std::string& value, bool& isNew);
    Node*  removeNode(Node* n, Key key, bool& removed);
    Node*  minNode(Node* n) const;
    void   rangeQuery(Node* n, Key lo, Key hi, std::vector<KeyValue>& out) const;
    void   destroyTree(Node* n);

    // Validation helpers
    bool   checkValid(Node* n, Key minKey, Key maxKey, bool hasMin, bool hasMax) const;
};

#endif  // MINIDB_AVL_INDEX_H
