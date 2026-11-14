#include "database.h"
#include "sorting.h"
#include <limits>
#include <stdexcept>
#include <utility>

Database::Database(const std::string& fileName,
                   std::unique_ptr<Index> indexToUse)
    : file(fileName), index(std::move(indexToUse)) {
    if (!index) {
        throw std::invalid_argument("Database needs an index");
    }
    // HOOK: persistence - load records from `file` into the index here.
}

Database::~Database() {
    // HOOK: persistence - save every record to `file` here.
}

bool Database::Put(Key key, const std::string& value) {
    std::string oldValue;
    bool existed = index->Get(key, &oldValue);
    index->Put(key, value);

    // Remember how to reverse this change.
    UndoEntry entry;
    entry.key = key;
    if (existed) {
        entry.action = UndoAction::RESTORE;
        entry.value = oldValue;
    } else {
        entry.action = UndoAction::REMOVE;
    }
    undoStack.Push(entry);

    // HOOK: cache - store (key, value) in the cache here.
    return !existed;
}

bool Database::Get(Key key, std::string* out) {
    // HOOK: cache - look in the cache first and return on a hit.
    bool found = index->Get(key, out);
    // HOOK: cache - on a miss that found the key, store it in the cache.
    return found;
}

bool Database::Delete(Key key) {
    std::string oldValue;
    if (!index->Get(key, &oldValue)) {
        return false;
    }
    index->Remove(key);

    UndoEntry entry;
    entry.action = UndoAction::RESTORE;
    entry.key = key;
    entry.value = oldValue;
    undoStack.Push(entry);

    // HOOK: cache - remove the key from the cache here.
    return true;
}

std::vector<KeyValue> Database::Scan(Key lo, Key hi) {
    // The cache is not used for range scans.
    return index->Range(lo, hi);
}

std::vector<KeyValue> Database::SortedByValue() {
    std::vector<KeyValue> records = index->Range(
        std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max());
    MergeSort(records, [](const KeyValue& a, const KeyValue& b) {
        if (a.second != b.second) {
            return a.second < b.second;
        }
        return a.first < b.first;
    });
    return records;
}

bool Database::Undo() {
    UndoEntry entry;
    if (!undoStack.Pop(&entry)) {
        return false;
    }
    // Apply directly to the index. This must NOT push a new undo entry.
    if (entry.action == UndoAction::RESTORE) {
        index->Put(entry.key, entry.value);
        // HOOK: cache - store the restored value in the cache here.
    } else {
        index->Remove(entry.key);
        // HOOK: cache - remove the key from the cache here.
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
