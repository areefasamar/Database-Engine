// ============================================================================
// lru_cache.cpp  -  implementation of LRUCache (see lru_cache.h for the idea)
// ============================================================================
#include "lru_cache.h"

LRUCache::LRUCache(size_t capacity)
    : capacity_(capacity),
      size_(0),
      hits_(0),
      misses_(0),
      head_(nullptr),
      tail_(nullptr) {}

// Walk the list from head to tail and free every node.
// (The HashTable frees its own memory in its own destructor.)
LRUCache::~LRUCache() {
    Node* cur = head_;
    while (cur != nullptr) {
        Node* next = cur->next;  // save before deleting
        delete cur;
        cur = next;
    }
}

// Remove n from the list by joining its neighbours to each other.
//   before:  A <-> n <-> B        after:  A <-> B
// Special cases: n is the head (no prev) or the tail (no next).
void LRUCache::Unlink(Node* n) {
    if (n->prev != nullptr) n->prev->next = n->next;
    else head_ = n->next;               // n was the head

    if (n->next != nullptr) n->next->prev = n->prev;
    else tail_ = n->prev;               // n was the tail

    n->prev = nullptr;                  // n is now a free-floating node
    n->next = nullptr;
}

// Insert n at the front of the list (it becomes the most recently used).
void LRUCache::PushFront(Node* n) {
    n->prev = nullptr;
    n->next = head_;                    // old head comes after n
    if (head_ != nullptr) head_->prev = n;
    head_ = n;
    if (tail_ == nullptr) tail_ = n;    // list was empty: n is head AND tail
}

// Remove the oldest entry.
// ORDER MATTERS: erase from the map first, then unlink, then delete.
// (Deleting first would leave a dangling pointer in the map.)
void LRUCache::EvictTail() {
    Node* victim = tail_;
    if (victim == nullptr) return;
    map_.Remove(victim->key);
    Unlink(victim);
    delete victim;
    --size_;
}

bool LRUCache::Get(Key key, std::string* out) {
    Node* n = nullptr;
    if (!map_.Get(key, &n)) {           // not in cache
        ++misses_;
        return false;
    }
    ++hits_;
    Unlink(n);                          // used just now -> move to the front
    PushFront(n);
    if (out != nullptr) *out = n->value;
    return true;
}

void LRUCache::Put(Key key, const std::string& value) {
    if (capacity_ == 0) return;         // a cache of size 0 stores nothing

    Node* n = nullptr;
    if (map_.Get(key, &n)) {            // key exists: update and move to front
        n->value = value;
        Unlink(n);
        PushFront(n);
        return;
    }

    if (size_ == capacity_) EvictTail();  // full: make room first

    n = new Node{key, value, nullptr, nullptr};
    PushFront(n);
    map_.Put(key, n);
    ++size_;
}

void LRUCache::Remove(Key key) {
    Node* n = nullptr;
    if (!map_.Get(key, &n)) return;     // not cached: nothing to do
    map_.Remove(key);
    Unlink(n);
    delete n;
    --size_;
}
