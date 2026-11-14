#include "avl_index.h"
#include "database.h"
#include "linked_list_index.h"
#include "sorted_array_index.h"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

using namespace std;

// Checks stay active even if NDEBUG is enabled.
void check(bool condition, const char* message) {
    if (!condition) {
        cerr << "FAILED: " << message << endl;
        exit(1);
    }
}

typedef function<unique_ptr<Index>()> IndexFactory;

unique_ptr<Index> makeList() { return unique_ptr<Index>(new LinkedListIndex()); }
unique_ptr<Index> makeArray() { return unique_ptr<Index>(new SortedArrayIndex()); }
unique_ptr<Index> makeAvl() { return unique_ptr<Index>(new AVLIndex()); }

void testBasics(const IndexFactory& make) {
    Database db("unused.db", make());
    string value = "unchanged";
    check(db.Size() == 0 && db.UndoDepth() == 0, "new database is empty");
    check(!db.Get(1, &value) && value == "unchanged", "get on empty database");
    check(!db.Delete(1), "delete on empty database");
    check(db.FileName() == "unused.db", "file name is kept");

    check(db.Put(101, "Areefa"), "new key returns true");
    check(!db.Put(101, "Areefa Samar") && db.Size() == 1, "update returns false");
    check(db.Get(101, &value) && value == "Areefa Samar", "updated value");
    check(db.Put(102, "") && db.Put(-5, "negative"), "empty value and negative key");
    check(db.Get(102, &value) && value.empty(), "empty value reads back");
    check(db.Size() == 3, "size after puts");

    check(db.Delete(101), "delete existing");
    check(!db.Delete(101), "delete twice");
    check(!db.Get(101, &value), "deleted key is gone");
    check(db.Size() == 2, "size after delete");
}

void testScanAndSort(const IndexFactory& make) {
    Database db("unused.db", make());
    db.Put(5, "pear");
    db.Put(1, "apple");
    db.Put(9, "fig");
    db.Put(3, "apple");
    db.Put(7, "");
    db.Put(2, "fig");

    vector<KeyValue> scan = db.Scan(2, 7);
    vector<KeyValue> expectedScan = {{2, "fig"}, {3, "apple"}, {5, "pear"}, {7, ""}};
    check(scan == expectedScan, "scan is inclusive and ordered by key");
    check(db.Scan(7, 2).empty(), "reversed scan is empty");
    check(db.Scan(100, 200).empty(), "scan with no matches");

    vector<KeyValue> sorted = db.SortedByValue();
    vector<KeyValue> expectedSorted = {
        {7, ""}, {1, "apple"}, {3, "apple"}, {2, "fig"}, {9, "fig"}, {5, "pear"}};
    check(sorted == expectedSorted, "sort by value, ties by key");
    check(db.Size() == 6, "sorting does not change the database");
    check(db.Scan(numeric_limits<Key>::min(), numeric_limits<Key>::max()).size() == 6,
          "records still in the index");

    Database empty("unused.db", make());
    check(empty.SortedByValue().empty(), "sort on an empty database");
}

void testUndo(const IndexFactory& make) {
    Database db("unused.db", make());
    string value;

    check(!db.Undo(), "undo with nothing to undo");

    // Undo a Put of a new key.
    db.Put(1, "one");
    check(db.UndoDepth() == 1, "one undo step");
    check(db.Undo() && !db.Get(1, &value) && db.Size() == 0, "undo new put");
    check(db.UndoDepth() == 0, "undo uses up the step");

    // Undo an update.
    db.Put(1, "one");
    db.Put(1, "uno");
    check(db.Undo() && db.Get(1, &value) && value == "one", "undo update");

    // Undo a delete.
    check(db.Delete(1) && !db.Get(1, &value), "deleted");
    check(db.Undo() && db.Get(1, &value) && value == "one", "undo delete");

    // Undo does not itself become an undo step.
    size_t depth = db.UndoDepth();
    db.Put(2, "two");
    check(db.UndoDepth() == depth + 1, "put adds a step");
    db.Undo();
    check(db.UndoDepth() == depth, "undo removes a step and adds none");

    // A failed delete adds no step.
    check(!db.Delete(999) && db.UndoDepth() == depth, "failed delete adds no step");

    // Undo several changes in a row, newest first.
    Database chain("unused.db", make());
    chain.Put(1, "a");
    chain.Put(2, "b");
    chain.Put(1, "c");
    chain.Delete(2);
    check(chain.Undo() && chain.Get(2, &value) && value == "b", "undo delete of 2");
    check(chain.Undo() && chain.Get(1, &value) && value == "a", "undo update of 1");
    check(chain.Undo() && !chain.Get(2, &value), "undo put of 2");
    check(chain.Undo() && !chain.Get(1, &value) && chain.Size() == 0,
          "undo put of 1");
    check(!chain.Undo(), "nothing left to undo");
}

void testRejectsNullIndex() {
    bool threw = false;
    try {
        Database db("unused.db", unique_ptr<Index>());
    } catch (const invalid_argument&) {
        threw = true;
    }
    check(threw, "null index is rejected");
}

// Same scenario on every index: the answers must be identical.
vector<KeyValue> runScenario(const IndexFactory& make) {
    Database db("unused.db", make());
    for (Key key = 0; key < 50; ++key) {
        db.Put((key * 37) % 101, "v" + to_string(key % 7));
    }
    db.Delete(37);
    db.Put(74, "changed");
    db.Undo();
    db.Undo();
    db.Delete(0);
    return db.SortedByValue();
}

void testSameResultOnEveryIndex() {
    vector<KeyValue> a = runScenario(makeList);
    vector<KeyValue> b = runScenario(makeArray);
    vector<KeyValue> c = runScenario(makeAvl);
    check(a == b && b == c, "all index types give the same results");
    check(!a.empty(), "scenario produced records");
}

// Random Put / Delete / Undo against a std::map oracle. The oracle keeps a
// snapshot before every change so Undo can be checked exactly.
void testRandomWithUndo(const IndexFactory& make, unsigned seed) {
    mt19937 rng(seed);
    Database db("unused.db", make());
    map<Key, string> expected;
    vector<map<Key, string>> snapshots;

    for (int step = 0; step < 3000; ++step) {
        Key key = static_cast<Key>(rng() % 31);
        int action = static_cast<int>(rng() % 100);

        if (action < 45) {
            string value = "v" + to_string(rng() % 1000);
            snapshots.push_back(expected);
            bool isNew = expected.count(key) == 0;
            check(db.Put(key, value) == isNew, "random put result");
            expected[key] = value;
        } else if (action < 70) {
            bool existed = expected.count(key) == 1;
            if (existed) {
                snapshots.push_back(expected);
            }
            check(db.Delete(key) == existed, "random delete result");
            expected.erase(key);
        } else if (action < 85) {
            bool canUndo = !snapshots.empty();
            check(db.Undo() == canUndo, "random undo result");
            if (canUndo) {
                expected = snapshots.back();
                snapshots.pop_back();
            }
        } else {
            string value;
            auto found = expected.find(key);
            bool ok = db.Get(key, &value);
            check(ok == (found != expected.end()), "random get result");
            if (ok) {
                check(value == found->second, "random get value");
            }
        }

        check(db.Size() == expected.size(), "random size");
        check(db.UndoDepth() == snapshots.size(), "random undo depth");
        if (step % 100 == 0) {
            vector<KeyValue> all(expected.begin(), expected.end());
            check(db.Scan(numeric_limits<Key>::min(), numeric_limits<Key>::max())
                  == all, "random full scan");
        }
    }
}

int main() {
    const IndexFactory factories[] = {makeList, makeArray, makeAvl};
    for (const IndexFactory& make : factories) {
        testBasics(make);
        testScanAndSort(make);
        testUndo(make);
    }
    testRejectsNullIndex();
    testSameResultOnEveryIndex();
    testRandomWithUndo(makeList, 1);
    testRandomWithUndo(makeArray, 2);
    testRandomWithUndo(makeAvl, 3);
    cout << "All Database tests passed." << endl;
    return 0;
}
