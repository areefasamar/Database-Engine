#include "sorted_array_index.h"
#include <utility>

namespace {
const std::size_t kInitialCapacity = 16;
}

SortedArrayIndex::SortedArrayIndex()
    : data(new KeyValue[kInitialCapacity]), count(0),
      capacity(kInitialCapacity) {}

SortedArrayIndex::~SortedArrayIndex() {
    delete[] data;
}

std::size_t SortedArrayIndex::lowerBound(Key key) const {
    std::size_t low = 0;
    std::size_t high = count;
    while (low < high) {
        std::size_t mid = low + (high - low) / 2;
        if (data[mid].first < key) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return low;
}

void SortedArrayIndex::grow() {
    std::size_t newCapacity = capacity * 2;
    KeyValue* bigger = new KeyValue[newCapacity];
    for (std::size_t i = 0; i < count; ++i) {
        bigger[i] = std::move(data[i]);
    }
    delete[] data;
    data = bigger;
    capacity = newCapacity;
}

bool SortedArrayIndex::Put(Key key, const std::string& value) {
    std::size_t pos = lowerBound(key);
    if (pos < count && data[pos].first == key) {
        data[pos].second = value;
        return false;
    }
    if (count == capacity) {
        grow();
    }
    for (std::size_t i = count; i > pos; --i) {
        data[i] = std::move(data[i - 1]);
    }
    data[pos] = KeyValue(key, value);
    ++count;
    return true;
}

bool SortedArrayIndex::Get(Key key, std::string* out) const {
    std::size_t pos = lowerBound(key);
    if (pos >= count || data[pos].first != key) {
        return false;
    }
    if (out != nullptr) {
        *out = data[pos].second;
    }
    return true;
}

bool SortedArrayIndex::Remove(Key key) {
    std::size_t pos = lowerBound(key);
    if (pos >= count || data[pos].first != key) {
        return false;
    }
    for (std::size_t i = pos; i + 1 < count; ++i) {
        data[i] = std::move(data[i + 1]);
    }
    --count;
    data[count] = KeyValue();  // release the leftover string
    return true;
}

std::vector<KeyValue> SortedArrayIndex::Range(Key lo, Key hi) const {
    std::vector<KeyValue> result;
    if (lo > hi) {
        return result;
    }
    for (std::size_t i = lowerBound(lo); i < count && data[i].first <= hi; ++i) {
        result.push_back(data[i]);
    }
    return result;
}

std::size_t SortedArrayIndex::Size() const {
    return count;
}
