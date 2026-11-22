#include "database.h"
#include "../data_structures/sorting.h"
#include <limits>
#include <stdexcept>
#include <utility>

Database::Database(const std::string& fileName,
                   std::unique_ptr<Index> indexToUse)
    : Database(fileName, std::move(indexToUse), 16) {}

Database::Database(const std::string& fileName,
                   std::unique_ptr<Index> indexToUse,
                   std::size_t cacheCapacity)
    : file(fileName),
      index(std::move(indexToUse)),
      cache(cacheCapacity) {

    if (!index) {
        throw std::invalid_argument("Database needs an index");
    }

    // Load persisted records into the index and warm up the cache
    std::vector<KeyValue> loaded;
    if (Persistence::Load(file, &loaded)) {
        for (const KeyValue& kv : loaded) {
            index->Put(kv.first, kv.second);
            cache.Put(kv.first, kv.second);
        }
    }
}

Database::~Database() {
    Save();
}

bool Database::Put(Key key, const std::string& value) {
    std::string oldValue;
    bool existed = index->Get(key, &oldValue);
    index->Put(key, value);

    // Keep cache synchronized with the index
    cache.Put(key, value);

    // Remember how to reverse this change
    UndoEntry entry;
    entry.key = key;
    if (existed) {
        entry.action = UndoAction::RESTORE;
        entry.value  = oldValue;
    } else {
        entry.action = UndoAction::REMOVE;
    }
    undoStack.Push(entry);

    return !existed;
}

bool Database::Get(Key key, std::string* out) {
    // 1) Fast path: check LRU cache first (O(1))
    if (cache.Get(key, out)) {
        return true;
    }

    // 2) Cache miss: look in the tree index (O(log N))
    std::string val;
    if (index->Get(key, &val)) {
        cache.Put(key, val);  // warm the cache for future gets
        if (out != nullptr) *out = val;
        return true;
    }

    return false;
}

bool Database::Delete(Key key) {
    std::string oldValue;
    if (!index->Get(key, &oldValue)) {
        return false;
    }

    index->Remove(key);
    cache.Remove(key);  // ensure cache never returns a deleted key

    UndoEntry entry;
    entry.action = UndoAction::RESTORE;
    entry.key    = key;
    entry.value  = oldValue;
    undoStack.Push(entry);

    return true;
}

std::vector<KeyValue> Database::Scan(Key lo, Key hi) {
    // Range scans read directly from the sorted index
    return index->Range(lo, hi);
}

std::vector<KeyValue> Database::SortedByValue() {
    std::vector<KeyValue> records =
        index->Range(std::numeric_limits<Key>::min(),
                     std::numeric_limits<Key>::max());

    MergeSort(records, [](const KeyValue& a, const KeyValue& b) {
        if (a.second != b.second) return a.second < b.second;
        return a.first < b.first;
    });

    return records;
}

bool Database::Undo() {
    UndoEntry entry;
    if (!undoStack.Pop(&entry)) {
        return false;
    }

    // Apply directly to the index and update cache (do NOT push a new undo entry)
    if (entry.action == UndoAction::RESTORE) {
        index->Put(entry.key, entry.value);
        cache.Put(entry.key, entry.value);
    } else {
        index->Remove(entry.key);
        cache.Remove(entry.key);
    }
    return true;
}

std::size_t Database::Size() const {
    return index->Size();
}

std::size_t Database::UndoDepth() const {
    return undoStack.Size();
}

const std::string& Database::FileName() const {
    return file;
}

bool Database::Save() {
    std::vector<KeyValue> records =
        index->Range(std::numeric_limits<Key>::min(),
                     std::numeric_limits<Key>::max());
    return Persistence::Save(file, records);
}
