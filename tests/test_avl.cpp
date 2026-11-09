#include "avl_index.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <type_traits>

using namespace std;

// Checks stay active even if NDEBUG is enabled.
void check(bool condition, const char* message) {
    if (!condition) {
        cerr << "FAILED: " << message << endl;
        exit(1);
    }
}

void checkContents(const AVLIndex& tree, const map<Key, string>& expected) {
    check(tree.IsValid(), "tree invariants");
    check(tree.Size() == expected.size(), "record count");
    vector<KeyValue> records(expected.begin(), expected.end());
    check(tree.Range(numeric_limits<Key>::min(), numeric_limits<Key>::max())
          == records, "all records match reference");
    for (const auto& record : expected) {
        string value;
        check(tree.Get(record.first, &value) && value == record.second,
              "lookup matches reference");
    }
}

void testBasics() {
    AVLIndex tree;
    string value = "unchanged";
    check(tree.Size() == 0 && tree.Height() == 0 && tree.IsValid(), "empty tree");
    check(!tree.Get(5, &value) && value == "unchanged", "missing lookup");
    check(!tree.Remove(5), "missing removal");
    check(tree.Range(-10, 10).empty(), "empty range");
    check(tree.Put(20, "first"), "new insertion");
    check(!tree.Put(20, "updated") && tree.Size() == 1, "update preserves size");
    check(tree.Get(20, &value) && value == "updated", "updated text");
    check(tree.Get(20, nullptr), "existence-only lookup");
    check(tree.Put(10, "") && tree.Put(30, "two words\nand a line"), "text values");
    check(tree.Range(20, 20) == vector<KeyValue>{{20, "updated"}}, "inclusive range");
    check(tree.Range(30, 10).empty(), "reversed range");
    check(tree.Range(11, 19).empty(), "gap range");
    check(tree.Remove(20), "two-child deletion");
    checkContents(tree, {{10, ""}, {30, "two words\nand a line"}});
    check(tree.Remove(30) && tree.Remove(10), "remove remaining nodes");
    check(tree.Height() == 0 && tree.Size() == 0 && tree.IsValid(), "empty after removal");

    Key low = numeric_limits<Key>::min();
    Key high = numeric_limits<Key>::max();
    tree.Put(low, "min");
    tree.Put(high, "max");
    tree.Put(0, "zero");
    checkContents(tree, {{low, "min"}, {0, "zero"}, {high, "max"}});
    check(tree.Range(high, high) == vector<KeyValue>{{high, "max"}}, "maximum key range");
}

void testRotationsAndDeletion() {
    const vector<vector<Key>> sequences = {
        {30, 20, 10}, {10, 20, 30}, {30, 10, 20}, {10, 30, 20},
        {30, 20, 40, 10, 25, 5}, {20, 10, 30, 25, 40, 50},
        {50, 20, 60, 10, 40, 30}, {20, 10, 50, 30, 60, 40}
    };
    for (const auto& keys : sequences) {
        AVLIndex tree;
        map<Key, string> expected;
        for (Key key : keys) {
            tree.Put(key, to_string(key));
            expected[key] = to_string(key);
            checkContents(tree, expected);
        }
        if (keys.size() == 3) {
            check(tree.Height() == 2, "three-node rotation height");
        }
        for (Key key : keys) {
            check(tree.Remove(key), "delete existing key");
            expected.erase(key);
            checkContents(tree, expected);
        }
    }

    // Deletion with a balanced heavy child needs a single rotation.
    for (int sign : {-1, 1}) {
        AVLIndex tree;
        for (Key key : {4, 2, 5, 1, 3}) {
            tree.Put(sign * key, "value");
        }
        check(tree.Remove(sign * 5) && tree.IsValid(), "deletion child balance zero");
        check(tree.Height() == 3 && tree.Size() == 4, "deletion rotation metadata");
    }
}

void testOrderedAndRandom() {
    mt19937 generator(2026);
    vector<Key> keys(1000);
    iota(keys.begin(), keys.end(), 1);
    for (int order = 0; order < 3; ++order) {
        if (order == 1) reverse(keys.begin(), keys.end());
        if (order == 2) shuffle(keys.begin(), keys.end(), generator);
        AVLIndex tree;
        map<Key, string> expected;
        for (Key key : keys) {
            check(tree.Put(key, to_string(key)), "bulk insertion");
            expected[key] = to_string(key);
            check(tree.IsValid(), "valid after each insertion");
        }
        check(tree.Height() <= 15, "1000-key height bound");
        checkContents(tree, expected);
        shuffle(keys.begin(), keys.end(), generator);
        for (Key key : keys) {
            check(tree.Remove(key), "random deletion");
            expected.erase(key);
            checkContents(tree, expected);
        }
        // Reset so the next pass really is reverse-sorted input.
        iota(keys.begin(), keys.end(), 1);
    }
}

void testMixedOperations() {
    AVLIndex tree;
    map<Key, string> expected; // Standard containers are test oracles only.
    mt19937 generator(42);
    uniform_int_distribution<int> keyDistribution(-500, 500);
    for (int step = 0; step < 100000; ++step) {
        Key key = keyDistribution(generator);
        int operation = static_cast<int>(generator() % 4);
        if (operation == 0) {
            string value = to_string(step);
            bool isNew = expected.find(key) == expected.end();
            check(tree.Put(key, value) == isNew, "mixed insertion result");
            expected[key] = value;
        }
        else if (operation == 1) {
            check(tree.Remove(key) == (expected.erase(key) != 0), "mixed deletion result");
        }
        else if (operation == 2) {
            string value;
            auto found = expected.find(key);
            check(tree.Get(key, &value) == (found != expected.end()), "mixed lookup result");
            if (found != expected.end()) check(value == found->second, "mixed lookup value");
        }
        else {
            Key high = keyDistribution(generator);
            vector<KeyValue> records;
            for (auto it = expected.lower_bound(key); it != expected.end() && it->first <= high; ++it) {
                records.push_back(*it);
            }
            check(tree.Range(key, high) == records, "mixed range");
        }
        check(tree.IsValid() && tree.Size() == expected.size(), "mixed invariants");
    }
    checkContents(tree, expected);
}

int main() {
    static_assert(!is_copy_constructible<AVLIndex>::value, "copying must be disabled");
    static_assert(!is_copy_assignable<AVLIndex>::value, "copy assignment must be disabled");
    testBasics();
    testRotationsAndDeletion();
    testOrderedAndRandom();
    testMixedOperations();
    cout << "All AVL tests passed (including 100000 mixed operations)." << endl;
}
