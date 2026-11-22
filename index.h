#ifndef MINIDB_INDEX_H
#define MINIDB_INDEX_H

// Abstract interface that every index implementation must satisfy.
// The Database class talks only to this interface, so the concrete
// index (LinkedListIndex, SortedArrayIndex, AVLIndex) is chosen by
// the caller and injected via the constructor.

#include "common.h"
#include <cstddef>
#include <string>
#include <vector>

class Index {
public:
    virtual ~Index() = default;

    // Insert or update. Returns true if the key is new, false if updated.
    virtual bool Put(Key key, const std::string& value) = 0;

    // Fills *out and returns true if key exists; returns false otherwise.
    // Pass nullptr for out if you only need the existence check.
    virtual bool Get(Key key, std::string* out) const = 0;

    // Removes the key. Returns false if it was not present.
    virtual bool Remove(Key key) = 0;

    // Returns all records with lo <= key <= hi, ordered by key.
    virtual std::vector<KeyValue> Range(Key lo, Key hi) const = 0;

    // Number of records currently stored.
    virtual std::size_t Size() const = 0;
};

#endif  // MINIDB_INDEX_H
