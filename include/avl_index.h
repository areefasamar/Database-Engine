#ifndef MINIDB_AVL_INDEX_H
#define MINIDB_AVL_INDEX_H

#include "index.h"

class AVLIndex : public Index {
private:
    class Node {
    public:
        Key key;
        std::string value;
        Node* left;
        Node* right;
        int height;

        Node(Key k, const std::string& v)
            : key(k), value(v), left(nullptr),
              right(nullptr), height(1) {}
    };

    Node* root;
    std::size_t count;

     void clearTree(Node* node);

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

    int Height() const;
    bool IsValid() const;
};

#endif