#include "linked_list_index.h"
#include <utility>

LinkedListIndex::LinkedListIndex() : head(nullptr), count(0) {}

LinkedListIndex::~LinkedListIndex() {
    while (head != nullptr) {
        Node* next = head->next;
        delete head;
        head = next;
    }
}

LinkedListIndex::Node* LinkedListIndex::findNode(Key key) const {
    for (Node* current = head; current != nullptr; current = current->next) {
        if (current->key == key) {
            return current;
        }
    }
    return nullptr;
}

bool LinkedListIndex::Put(Key key, const std::string& value) {
    Node* existing = findNode(key);
    if (existing != nullptr) {
        existing->value = value;
        return false;
    }
    head = new Node(key, value, head);
    ++count;
    return true;
}

bool LinkedListIndex::Get(Key key, std::string* out) const {
    Node* node = findNode(key);
    if (node == nullptr) {
        return false;
    }
    if (out != nullptr) {
        *out = node->value;
    }
    return true;
}

bool LinkedListIndex::Remove(Key key) {
    Node* previous = nullptr;
    for (Node* current = head; current != nullptr; current = current->next) {
        if (current->key == key) {
            if (previous == nullptr) {
                head = current->next;
            } else {
                previous->next = current->next;
            }
            delete current;
            --count;
            return true;
        }
        previous = current;
    }
    return false;
}

std::vector<KeyValue> LinkedListIndex::Range(Key lo, Key hi) const {
    std::vector<KeyValue> result;
    if (lo > hi) {
        return result;
    }
    for (Node* current = head; current != nullptr; current = current->next) {
        if (current->key >= lo && current->key <= hi) {
            result.push_back(KeyValue(current->key, current->value));
        }
    }

    // The list is unsorted, so order the matches by key (insertion sort).
    for (std::size_t i = 1; i < result.size(); ++i) {
        KeyValue item = std::move(result[i]);
        std::size_t j = i;
        while (j > 0 && result[j - 1].first > item.first) {
            result[j] = std::move(result[j - 1]);
            --j;
        }
        result[j] = std::move(item);
    }
    return result;
}

std::size_t LinkedListIndex::Size() const {
    return count;
}
