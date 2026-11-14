#ifndef MINIDB_LINKED_LIST_INDEX_H
#define MINIDB_LINKED_LIST_INDEX_H

#include "index.h"

// Baseline index: an unsorted singly linked list searched linearly, O(N).
// Used for the Level 1 demo, as the benchmark baseline, and as a reference
// for testing the AVL tree.
class LinkedListIndex : public Index {
private:
    class Node {
    public:
        Key key;
        std::string value;
        Node* next;

        Node(Key k, const std::string& v, Node* n)
            : key(k), value(v), next(n) {}
    };

    Node* head;
    std::size_t count;

    Node* findNode(Key key) const;

public:
    LinkedListIndex();
    ~LinkedListIndex() override;

    LinkedListIndex(const LinkedListIndex&) = delete;
    LinkedListIndex& operator=(const LinkedListIndex&) = delete;

    bool Put(Key key, const std::string& value) override;
    bool Get(Key key, std::string* out) const override;
    bool Remove(Key key) override;
    std::vector<KeyValue> Range(Key lo, Key hi) const override;
    std::size_t Size() const override;
};

#endif
