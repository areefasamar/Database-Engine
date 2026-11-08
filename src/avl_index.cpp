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