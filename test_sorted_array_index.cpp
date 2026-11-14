#include "sorted_array_index.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>

using namespace std;

// Checks stay active even if NDEBUG is enabled.
void check(bool condition, const char* message) {
    if (!condition) {
        cerr << "FAILED: " << message << endl;
        exit(1);
    }
}

void checkContents(const Index& index, const map<Key, string>& expected) {
    check(index.Size() == expected.size(), "record count");
    vector<KeyValue> records(expected.begin(), expected.end());
    check(index.Range(numeric_limits<Key>::min(), numeric_limits<Key>::max())
          == records, "all records match reference");
    for (const auto& record : expected) {
        string value;
        check(index.Get(record.first, &value) && value == record.second,
              "lookup matches reference");
    }
}

void testBasics() {
    SortedArrayIndex index;
    string value = "unchanged";
    check(index.Size() == 0, "empty index size");
    check(!index.Get(5, &value) && value == "unchanged", "missing lookup");
    check(!index.Remove(5), "missing removal");
    check(index.Range(-10, 10).empty(), "empty range");

    check(index.Put(20, "first"), "new insertion");
    check(!index.Put(20, "updated") && index.Size() == 1, "update keeps size");
    check(index.Get(20, &value) && value == "updated", "updated text");
    check(index.Get(20, nullptr), "existence-only lookup");
    check(!index.Get(21, nullptr), "existence-only miss");
    check(index.Put(10, "") && index.Put(30, "two words\nand a line"),
          "empty and multiline values");
    check(index.Get(10, &value) && value.empty(), "empty value read back");

    check(index.Range(20, 20) == vector<KeyValue>{{20, "updated"}}, "inclusive range");
    check(index.Range(10, 30).size() == 3, "range covers both endpoints");
    check(index.Range(30, 10).empty(), "reversed range");
    check(index.Range(11, 19).empty(), "gap range");

    check(index.Remove(20), "remove existing");
    check(!index.Remove(20), "remove twice");
    checkContents(index, {{10, ""}, {30, "two words\nand a line"}});
    check(index.Remove(30) && index.Remove(10), "remove remaining");
    check(index.Size() == 0 && index.Range(-100, 100).empty(), "empty after removal");
}

void testNegativeAndLimitKeys() {
    SortedArrayIndex index;
    Key low = numeric_limits<Key>::min();
    Key high = numeric_limits<Key>::max();
    index.Put(high, "max");
    index.Put(-5, "minus five");
    index.Put(low, "min");
    index.Put(0, "zero");
    checkContents(index, {{low, "min"}, {-5, "minus five"}, {0, "zero"}, {high, "max"}});
    check(index.Range(high, high) == vector<KeyValue>{{high, "max"}}, "max key range");
    check(index.Range(low, low) == vector<KeyValue>{{low, "min"}}, "min key range");
    check(index.Range(-5, 0).size() == 2, "negative to zero range");
}

void testRemovePositions() {
    SortedArrayIndex index;
    map<Key, string> expected;
    for (Key key = 1; key <= 5; ++key) {
        index.Put(key, "v" + to_string(key));
        expected[key] = "v" + to_string(key);
    }
    check(index.Remove(5), "remove last element");
    expected.erase(5);
    checkContents(index, expected);
    check(index.Remove(1), "remove first element");
    expected.erase(1);
    checkContents(index, expected);
    check(index.Remove(3), "remove middle element");
    expected.erase(3);
    checkContents(index, expected);
    check(!index.Remove(99), "remove absent key");
    checkContents(index, expected);
}

// The array starts small and doubles, so insert far past the first capacity
// at the front, the back and the middle.
void testGrowth() {
    SortedArrayIndex index;
    map<Key, string> expected;
    for (Key key = 500; key >= 1; --key) {
        index.Put(key * 2, "front");
        expected[key * 2] = "front";
    }
    for (Key key = 1; key <= 500; ++key) {
        index.Put(1000 + key, "back");
        expected[1000 + key] = "back";
    }
    for (Key key = 1; key <= 500; ++key) {
        index.Put(key * 2 - 1, "middle");
        expected[key * 2 - 1] = "middle";
    }
    checkContents(index, expected);
}

void testInsertionOrders() {
    vector<Key> keys(1000);
    iota(keys.begin(), keys.end(), 1);

    for (int order = 0; order < 3; ++order) {
        if (order == 1) {
            reverse(keys.begin(), keys.end());
        } else if (order == 2) {
            mt19937 rng(7);
            shuffle(keys.begin(), keys.end(), rng);
        }
        SortedArrayIndex index;
        map<Key, string> expected;
        for (Key key : keys) {
            check(index.Put(key, "value " + to_string(key)), "insert new key");
            expected[key] = "value " + to_string(key);
        }
        checkContents(index, expected);

        mt19937 rng(11);
        shuffle(keys.begin(), keys.end(), rng);
        for (size_t i = 0; i < keys.size(); ++i) {
            check(index.Remove(keys[i]), "remove inserted key");
            expected.erase(keys[i]);
            if (i % 100 == 0) {
                checkContents(index, expected);
            }
        }
        check(index.Size() == 0, "empty after removing everything");
        sort(keys.begin(), keys.end());
    }
}

void testRandomWorkload() {
    mt19937 rng(12345);
    SortedArrayIndex index;
    map<Key, string> expected;

    for (int step = 0; step < 4000; ++step) {
        Key key = static_cast<Key>(rng() % 201) - 100;
        int action = static_cast<int>(rng() % 100);
        if (action < 40) {
            string value = "v" + to_string(rng());
            bool isNew = expected.count(key) == 0;
            check(index.Put(key, value) == isNew, "random put result");
            expected[key] = value;
        } else if (action < 65) {
            bool existed = expected.erase(key) == 1;
            check(index.Remove(key) == existed, "random remove result");
        } else if (action < 85) {
            string value;
            auto found = expected.find(key);
            bool ok = index.Get(key, &value);
            check(ok == (found != expected.end()), "random get result");
            if (ok) {
                check(value == found->second, "random get value");
            }
        } else {
            Key other = static_cast<Key>(rng() % 221) - 110;
            vector<KeyValue> want;
            for (auto it = expected.lower_bound(key);
                 it != expected.end() && it->first <= other; ++it) {
                want.push_back(*it);
            }
            check(index.Range(key, other) == want, "random range result");
        }
        if (step % 250 == 0) {
            checkContents(index, expected);
        }
    }
    checkContents(index, expected);
}

int main() {
    testBasics();
    testNegativeAndLimitKeys();
    testRemovePositions();
    testGrowth();
    testInsertionOrders();
    testRandomWorkload();
    cout << "All SortedArrayIndex tests passed." << endl;
    return 0;
}
