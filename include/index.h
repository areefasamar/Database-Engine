#ifndef MINIDB_INDEX_H
#define MINIDB_INDEX_H

#include "common.h"
#include <cstddef>
#include <vector>

class Index {
public:
    virtual ~Index() = default;

    // Inserts or updates a record. Returns true only for a new key.
    virtual bool Put(Key key, const std::string& value) = 0;
    virtual bool Get(Key key, std::string* out) const = 0;
    virtual bool Remove(Key key) = 0;

    // Returns records in the inclusive range, ordered by key.
    virtual std::vector<KeyValue> Range(Key lo, Key hi) const = 0;
    virtual std::size_t Size() const = 0;
};

#endif
