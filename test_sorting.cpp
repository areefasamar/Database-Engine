#include "common.h"
#include "sorting.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace std;

// Checks stay active even if NDEBUG is enabled.
void check(bool condition, const char* message) {
    if (!condition) {
        cerr << "FAILED: " << message << endl;
        exit(1);
    }
}

typedef void (*IntSort)(vector<int>&);

struct NamedSort {
    const char* name;
    IntSort sort;
    bool quadratic;  // too slow for the big inputs
};

void bubble(vector<int>& v) { BubbleSort(v); }
void insertion(vector<int>& v) { InsertionSort(v); }
void mergeSort(vector<int>& v) { MergeSort(v); }
void quick(vector<int>& v) { QuickSort(v); }

const NamedSort kSorts[] = {
    {"BubbleSort", bubble, true},
    {"InsertionSort", insertion, true},
    {"MergeSort", mergeSort, false},
    {"QuickSort", quick, false},
};

void checkSorts(const vector<int>& input, bool includeQuadratic,
                const char* label) {
    vector<int> expected = input;
    sort(expected.begin(), expected.end());  // std::sort only as a checker
    for (const NamedSort& s : kSorts) {
        if (s.quadratic && !includeQuadratic) {
            continue;
        }
        vector<int> data = input;
        s.sort(data);
        if (data != expected) {
            cerr << "FAILED: " << s.name << " on " << label << endl;
            exit(1);
        }
    }
}

void testSmallCases() {
    checkSorts({}, true, "empty");
    checkSorts({42}, true, "one element");
    checkSorts({1, 2}, true, "two sorted");
    checkSorts({2, 1}, true, "two reversed");
    checkSorts({2, 1, 3}, true, "three");
    checkSorts({5, 5, 5, 5, 5}, true, "all equal");
    checkSorts({3, 1, 3, 2, 1, 2, 3}, true, "duplicates");
    checkSorts({-5, 10, -20, 0, 7, -1}, true, "negatives");
}

void testPatterns() {
    vector<int> ascending(1000);
    iota(ascending.begin(), ascending.end(), 0);
    checkSorts(ascending, true, "already sorted");

    vector<int> descending(ascending.rbegin(), ascending.rend());
    checkSorts(descending, true, "reverse sorted");

    mt19937 rng(5);
    for (int round = 0; round < 20; ++round) {
        vector<int> random(1000);
        for (int& x : random) {
            x = static_cast<int>(rng() % 2000) - 1000;
        }
        checkSorts(random, true, "random");
    }
    for (int round = 0; round < 20; ++round) {
        vector<int> few(1000);
        for (int& x : few) {
            x = static_cast<int>(rng() % 5);
        }
        checkSorts(few, true, "few distinct values");
    }

    // Organ pipe: rises then falls. A classic hard case for naive quick sort.
    vector<int> pipe;
    for (int i = 0; i < 500; ++i) pipe.push_back(i);
    for (int i = 500; i > 0; --i) pipe.push_back(i);
    checkSorts(pipe, true, "organ pipe");
}

// Large already-sorted, reverse and all-equal inputs must not blow up
// (this is what median-of-three and Hoare partitioning protect against).
void testLargeInputsStayFast() {
    const int n = 200000;
    vector<int> sorted(n);
    iota(sorted.begin(), sorted.end(), 0);
    vector<int> reversed(sorted.rbegin(), sorted.rend());
    vector<int> equal(n, 7);
    checkSorts(sorted, false, "large sorted");
    checkSorts(reversed, false, "large reversed");
    checkSorts(equal, false, "large all equal");
}

struct Tagged {
    int key;
    int order;  // original position
};

bool tagLess(const Tagged& a, const Tagged& b) { return a.key < b.key; }

void checkStable(const char* name, void (*run)(vector<Tagged>&)) {
    mt19937 rng(77);
    vector<Tagged> data(500);
    for (int i = 0; i < 500; ++i) {
        data[i].key = static_cast<int>(rng() % 10);  // lots of equal keys
        data[i].order = i;
    }
    run(data);
    for (size_t i = 1; i < data.size(); ++i) {
        bool inOrder = data[i - 1].key < data[i].key
                       || (data[i - 1].key == data[i].key
                           && data[i - 1].order < data[i].order);
        if (!inOrder) {
            cerr << "FAILED: " << name << " is not stable" << endl;
            exit(1);
        }
    }
}

void testStability() {
    checkStable("BubbleSort", [](vector<Tagged>& v) { BubbleSort(v, tagLess); });
    checkStable("InsertionSort", [](vector<Tagged>& v) { InsertionSort(v, tagLess); });
    checkStable("MergeSort", [](vector<Tagged>& v) { MergeSort(v, tagLess); });
}

void testComparatorsAndTypes() {
    vector<int> data = {4, 9, 1, 7, 3, 8};
    vector<int> descending = {9, 8, 7, 4, 3, 1};
    for (int which = 0; which < 4; ++which) {
        vector<int> copy = data;
        greater<int> cmp;  // used only as a comparator
        if (which == 0) BubbleSort(copy, cmp);
        if (which == 1) InsertionSort(copy, cmp);
        if (which == 2) MergeSort(copy, cmp);
        if (which == 3) QuickSort(copy, cmp);
        check(copy == descending, "descending comparator");
    }

    vector<string> words = {"pear", "apple", "fig", "banana", "apple", ""};
    vector<string> expected = {"", "apple", "apple", "banana", "fig", "pear"};
    vector<string> w = words;
    MergeSort(w);
    check(w == expected, "strings with merge sort");
    w = words;
    QuickSort(w);
    check(w == expected, "strings with quick sort");

    // Raw array form.
    int raw[] = {5, 2, 9, 1, 5, 6};
    MergeSort(raw, 6, DefaultLess());
    check(is_sorted(begin(raw), end(raw)), "raw array merge sort");
    int raw2[] = {5, 2, 9, 1, 5, 6};
    QuickSort(raw2, 6, DefaultLess());
    check(is_sorted(begin(raw2), end(raw2)), "raw array quick sort");
}

// The exact ordering the SORT VALUE command will use: by value, ties by key.
void testKeyValueByValue() {
    vector<KeyValue> records = {
        {5, "pear"}, {1, "apple"}, {9, "fig"}, {3, "apple"}, {7, ""}, {2, "fig"}};
    auto byValue = [](const KeyValue& a, const KeyValue& b) {
        if (a.second != b.second) {
            return a.second < b.second;
        }
        return a.first < b.first;
    };
    vector<KeyValue> expected = {
        {7, ""}, {1, "apple"}, {3, "apple"}, {2, "fig"}, {9, "fig"}, {5, "pear"}};
    vector<KeyValue> a = records;
    MergeSort(a, byValue);
    check(a == expected, "KeyValue sorted by value with merge sort");
    vector<KeyValue> b = records;
    QuickSort(b, byValue);
    check(b == expected, "KeyValue sorted by value with quick sort");
}

void testRandomAgreement() {
    mt19937 rng(31337);
    for (int round = 0; round < 200; ++round) {
        size_t n = rng() % 300;
        int spread = 1 + static_cast<int>(rng() % 100);
        vector<int> input(n);
        for (int& x : input) {
            x = static_cast<int>(rng() % spread) - spread / 2;
        }
        checkSorts(input, true, "random agreement");
    }
}

int main() {
    testSmallCases();
    testPatterns();
    testLargeInputsStayFast();
    testStability();
    testComparatorsAndTypes();
    testKeyValueByValue();
    testRandomAgreement();
    cout << "All sorting tests passed." << endl;
    return 0;
}
