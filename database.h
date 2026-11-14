#ifndef MINIDB_DATABASE_H
#define MINIDB_DATABASE_H

#include "index.h"
#include "stack.h"
#include <memory>
#include <string>
#include <vector>

// The single public API that ties the engine together.
// It talks only to the Index interface, so the index implementation
// (LinkedListIndex, SortedArrayIndex, AVLIndex) is chosen by the caller.
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

public:
    // Takes ownership of the index (must not be null).
    Database(const std::string& fileName, std::unique_ptr<Index> indexToUse);
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
};

#endif
