#pragma once
// ============================================================================
// hash_table.h  -  our own hash table (separate chaining + rehashing)
//
// Idea:
//   - An array of "buckets". Each bucket is a singly linked list (a chain).
//   - A hash function turns a key into a bucket number.
//   - Two keys in the same bucket = collision -> they sit in the same chain.
//   - When the table gets too full (load factor > 0.75) we REHASH:
//     build a bigger array and move every node into it.
//
// It is a template (HashTable<V>) so the whole code must live in this header.
// ============================================================================
#include <cstddef>   // size_t
#include <cstdint>   // uint64_t

#include "../common.h"  // defines Key (int)

template <typename V>
class HashTable {
public:
    // Start with 17 buckets (a prime number). "()" at the end sets every
    // bucket pointer to nullptr, i.e. every chain starts empty.
    HashTable()
        : buckets_(new Node*[kInitialBuckets]()),
          bucket_count_(kInitialBuckets),
          size_(0) {}

    // Free every node of every chain, then free the bucket array itself.
    // Without this loop we would leak memory.
    ~HashTable() {
        for (size_t i = 0; i < bucket_count_; ++i) {
            Node* cur = buckets_[i];
            while (cur != nullptr) {
                Node* next = cur->next;  // save next BEFORE deleting cur
                delete cur;
                cur = next;
            }
        }
        delete[] buckets_;
    }

    // Copying would make two tables free the same nodes (double free),
    // so copying is switched off.
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    // Insert or update.
    // Returns true  -> key was new (inserted)
    // Returns false -> key already existed (value updated)
    bool Put(Key key, const V& value) {
        size_t idx = IndexFor(key, bucket_count_);

        // 1) Walk the chain: if the key exists, just update its value.
        for (Node* cur = buckets_[idx]; cur != nullptr; cur = cur->next) {
            if (cur->key == key) {
                cur->value = value;
                return false;
            }
        }

        // 2) Key not found: add a new node at the FRONT of the chain (O(1)).
        //    New node's next = old first node, then bucket points to new node.
        Node* node = new Node{key, value, buckets_[idx]};
        buckets_[idx] = node;
        ++size_;

        // 3) Too full? Grow the table so chains stay short.
        if (LoadFactor() > kMaxLoad) Rehash();
        return true;
    }

    // Look up a key. If found, copies the value into *out and returns true.
    // const: reading does not change the table.
    bool Get(Key key, V* out) const {
        size_t idx = IndexFor(key, bucket_count_);
        for (Node* cur = buckets_[idx]; cur != nullptr; cur = cur->next) {
            if (cur->key == key) {
                if (out != nullptr) *out = cur->value;
                return true;
            }
        }
        return false;  // walked the whole chain, key is not here
    }

    // Delete a key. Returns true if it existed.
    bool Remove(Key key) {
        size_t idx = IndexFor(key, bucket_count_);
        Node* prev = nullptr;          // node just before cur (needed to unlink)
        Node* cur = buckets_[idx];
        while (cur != nullptr) {
            if (cur->key == key) {
                if (prev == nullptr) {
                    buckets_[idx] = cur->next;  // cur was FIRST: bucket now points to next
                } else {
                    prev->next = cur->next;     // cur was MIDDLE/LAST: skip over it
                }
                delete cur;
                --size_;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    size_t Size() const { return size_; }                 // number of keys stored
    size_t BucketCount() const { return bucket_count_; }  // length of bucket array
    double LoadFactor() const {                           // size / buckets
        return static_cast<double>(size_) / static_cast<double>(bucket_count_);
    }

private:
    // One element of a chain.
    struct Node {
        Key key;
        V value;
        Node* next;  // next node in the same bucket (nullptr = end of chain)
    };

    static constexpr size_t kInitialBuckets = 17;  // prime
    static constexpr double kMaxLoad = 0.75;       // rehash above this

    // The hash function: key -> bucket number.
    // Cast to UNSIGNED first: in C++ a negative number % n gives a negative
    // result, which would be an invalid array index.
    static size_t IndexFor(Key key, size_t buckets) {
        return static_cast<size_t>(static_cast<uint64_t>(key) %
                                   static_cast<uint64_t>(buckets));
    }

    // Helpers to pick a prime bucket count (primes spread keys more evenly).
    static bool IsPrime(size_t n) {
        if (n < 2) return false;
        for (size_t d = 2; d * d <= n; ++d) {
            if (n % d == 0) return false;
        }
        return true;
    }

    static size_t NextPrime(size_t n) {
        while (!IsPrime(n)) ++n;
        return n;
    }

    // Grow the table: about double the buckets, then move every node.
    // We RELINK existing nodes (no copying, no new/delete), so no leaks.
    // Bucket numbers change because the modulus changed, so every node
    // must be placed again using the new bucket count.
    void Rehash() {
        size_t new_count = NextPrime(bucket_count_ * 2 + 1);
        Node** fresh = new Node*[new_count]();  // all nullptr

        for (size_t i = 0; i < bucket_count_; ++i) {
            Node* cur = buckets_[i];
            while (cur != nullptr) {
                Node* next = cur->next;                 // save BEFORE changing cur->next
                size_t idx = IndexFor(cur->key, new_count);
                cur->next = fresh[idx];                 // push cur onto the new chain
                fresh[idx] = cur;
                cur = next;
            }
        }

        delete[] buckets_;        // old array only (nodes now live in `fresh`)
        buckets_ = fresh;
        bucket_count_ = new_count;
    }

    Node** buckets_;       // array of pointers; buckets_[i] = first node of chain i
    size_t bucket_count_;  // how many buckets
    size_t size_;          // how many keys in total
};
