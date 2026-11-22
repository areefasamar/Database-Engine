#ifndef MINIDB_DATABASE_H
#define MINIDB_DATABASE_H

#include "../index.h"
#include "../data_structures/stack.h"
#include "../cache/lru_cache.h"
#include "persistence.h"
#include <memory>
#include <string>
#include <vector>

// The single public API that ties the engine together.
// It integrates:
//   - Index (AVL tree, sorted array, or linked list)
//   - LRU Cache (HashTable + doubly-linked list for O(1) reads)
//   - Undo Stack (Stack<UndoEntry> for rollback)
//   - Persistence (atomic disk save and load)
class Database {
private:
    // What UNDO has to do to reverse one change.
    enum class UndoAction { REMOVE, RESTORE };

    struct UndoEntry {
        UndoAction action = UndoAction::REMOVE;
        Key key = 0;
        std::string value;  // RESTORE: the value to put back
    };

    std::string file;
    std::unique_ptr<Index> index;
    Stack<UndoEntry> undoStack;
    LRUCache cache;

public:
    // Takes ownership of the index (must not be null).
    Database(const std::string& fileName,
             std::unique_ptr<Index> indexToUse);
    Database(const std::string& fileName,
             std::unique_ptr<Index> indexToUse,
             std::size_t cacheCapacity);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Returns true only when the key is new (false means it was updated).
    bool Put(Key key, const std::string& value);
    bool Get(Key key, std::string* out);
    // Returns false if the key does not exist.
    bool Delete(Key key);

    // Records with lo <= key <= hi, ordered by key.
    std::vector<KeyValue> Scan(Key lo, Key hi);
    // All records ordered by value (ties broken by key), using merge sort.
    std::vector<KeyValue> SortedByValue();

    // Reverses the last Put or Delete. Returns false if there is nothing to undo.
    bool Undo();

    std::size_t Size() const;
    std::size_t UndoDepth() const;
    const std::string& FileName() const;

    // Cache metrics (for STATS)
    std::size_t CacheHits() const     { return cache.Hits(); }
    std::size_t CacheMisses() const   { return cache.Misses(); }
    std::size_t CacheSize() const     { return cache.Size(); }
    std::size_t CacheCapacity() const { return cache.Capacity(); }
    std::string CacheStats() const {
        return "hits: " + std::to_string(cache.Hits()) + ", misses: " + std::to_string(cache.Misses());
    }

    // Manually flush all records to disk right now.
    // Destructor calls this automatically on EXIT.
    bool Save();
};

#endif
