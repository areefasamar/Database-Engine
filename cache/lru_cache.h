#pragma once
// ============================================================================
// lru_cache.h  -  declaration of LRUCache
//
// Uses HashTable<Node*> for O(1) lookup and a doubly-linked list for O(1)
// eviction of least-recently-used records.
// ============================================================================
#include <cstddef>
#include <string>
#include "hash_table.h"
#include "../common.h"

class LRUCache {
public:
    explicit LRUCache(size_t capacity = 16);
    ~LRUCache();

    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    bool Get(Key key, std::string* out);
    void Put(Key key, const std::string& value);
    void Remove(Key key);

    size_t Hits() const { return hits_; }
    size_t Misses() const { return misses_; }
    size_t Size() const { return size_; }
    size_t Capacity() const { return capacity_; }

private:
    struct Node {
        Key key;
        std::string value;
        Node* prev;
        Node* next;
    };

    void Unlink(Node* n);
    void PushFront(Node* n);
    void EvictTail();

    size_t capacity_;
    size_t size_;
    size_t hits_;
    size_t misses_;
    Node* head_;
    Node* tail_;
    HashTable<Node*> map_;
};
