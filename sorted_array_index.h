#ifndef MINIDB_SORTED_ARRAY_INDEX_H
#define MINIDB_SORTED_ARRAY_INDEX_H

#include "index.h"

// Baseline index: a dynamic array kept sorted by key.
// Lookup uses binary search, O(log N). Insert and remove shift elements, O(N).
class SortedArrayIndex : public Index {
private:
    KeyValue* data;
    std::size_t count;
    std::size_t capacity;

    // Index of the first element with key >= the given key (count if none).
    std::size_t lowerBound(Key key) const;
    void grow();

public:
    SortedArrayIndex();
    ~SortedArrayIndex() override;

    SortedArrayIndex(const SortedArrayIndex&) = delete;
    SortedArrayIndex& operator=(const SortedArrayIndex&) = delete;

    bool Put(Key key, const std::string& value) override;
    bool Get(Key key, std::string* out) const override;
    bool Remove(Key key) override;
    std::vector<KeyValue> Range(Key lo, Key hi) const override;
    std::size_t Size() const override;
};

#endif
