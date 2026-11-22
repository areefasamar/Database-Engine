#ifndef MINIDB_SORTING_H
#define MINIDB_SORTING_H

#include <cstddef>
#include <utility>
#include <vector>

// Sorting algorithms written from scratch (course weeks 6 and 7).
// Every sort takes a "less" comparator: less(a, b) is true if a goes before b.
// Each has a raw-array form (pointer and length) and a std::vector form.
// The vector forms without a comparator sort with operator<.

struct DefaultLess {
    template <typename T>
    bool operator()(const T& a, const T& b) const { return a < b; }
};

// ---------------------------------------------------------------- Bubble sort
// Stable. O(n^2) worst and average, O(n) on already sorted input.
template <typename T, typename Less>
void BubbleSort(T* items, std::size_t n, Less less) {
    if (n < 2) {
        return;
    }
    bool swapped = true;
    for (std::size_t end = n - 1; swapped && end > 0; --end) {
        swapped = false;
        for (std::size_t i = 0; i < end; ++i) {
            if (less(items[i + 1], items[i])) {
                std::swap(items[i], items[i + 1]);
                swapped = true;
            }
        }
    }
}

// ------------------------------------------------------------- Insertion sort
// Stable. O(n^2) worst and average, O(n) on already sorted input.
template <typename T, typename Less>
void InsertionSort(T* items, std::size_t n, Less less) {
    for (std::size_t i = 1; i < n; ++i) {
        T current = std::move(items[i]);
        std::size_t j = i;
        while (j > 0 && less(current, items[j - 1])) {
            items[j] = std::move(items[j - 1]);
            --j;
        }
        items[j] = std::move(current);
    }
}

// ----------------------------------------------------------------- Merge sort
// Stable. Always O(n log n). Uses one temporary buffer allocated once.
namespace sorting_detail {

template <typename T, typename Less>
void mergeSortRange(T* items, T* buffer, std::size_t lo, std::size_t hi,
                    Less less) {
    // Sorts the half-open range [lo, hi).
    if (hi - lo < 2) {
        return;
    }
    std::size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(items, buffer, lo, mid, less);
    mergeSortRange(items, buffer, mid, hi, less);

    std::size_t left = lo;
    std::size_t right = mid;
    std::size_t out = lo;
    while (left < mid && right < hi) {
        // Take from the right only if it is strictly smaller: keeps it stable.
        if (less(items[right], items[left])) {
            buffer[out++] = std::move(items[right++]);
        } else {
            buffer[out++] = std::move(items[left++]);
        }
    }
    while (left < mid) {
        buffer[out++] = std::move(items[left++]);
    }
    while (right < hi) {
        buffer[out++] = std::move(items[right++]);
    }
    for (std::size_t i = lo; i < hi; ++i) {
        items[i] = std::move(buffer[i]);
    }
}

// ------------------------------------------------------------------ Quick sort
// Median-of-three pivot and Hoare-style partitioning (stops on equal keys, so
// many duplicates stay fast). Recurses into the smaller side and loops on the
// larger, so stack depth is O(log n).
template <typename T, typename Less>
void quickSortRange(T* items, long lo, long hi, Less less) {
    while (lo < hi) {
        long mid = lo + (hi - lo) / 2;
        if (less(items[mid], items[lo])) {
            std::swap(items[mid], items[lo]);
        }
        if (less(items[hi], items[lo])) {
            std::swap(items[hi], items[lo]);
        }
        if (less(items[hi], items[mid])) {
            std::swap(items[hi], items[mid]);
        }
        T pivot = items[mid];  // the median of the three

        long i = lo;
        long j = hi;
        while (i <= j) {
            while (less(items[i], pivot)) {
                ++i;
            }
            while (less(pivot, items[j])) {
                --j;
            }
            if (i <= j) {
                std::swap(items[i], items[j]);
                ++i;
                --j;
            }
        }
        // Now [lo, j] holds items <= pivot and [i, hi] holds items >= pivot.
        if (j - lo < hi - i) {
            quickSortRange(items, lo, j, less);
            lo = i;
        } else {
            quickSortRange(items, i, hi, less);
            hi = j;
        }
    }
}

}  // namespace sorting_detail

template <typename T, typename Less>
void MergeSort(T* items, std::size_t n, Less less) {
    if (n < 2) {
        return;
    }
    T* buffer = new T[n];
    sorting_detail::mergeSortRange(items, buffer, 0, n, less);
    delete[] buffer;
}

// Not stable. O(n log n) on average, O(n^2) only with very unlucky input.
template <typename T, typename Less>
void QuickSort(T* items, std::size_t n, Less less) {
    if (n < 2) {
        return;
    }
    sorting_detail::quickSortRange(items, 0, static_cast<long>(n) - 1, less);
}

// ------------------------------------------------------------- vector wrappers
template <typename T, typename Less>
void BubbleSort(std::vector<T>& v, Less less) { BubbleSort(v.data(), v.size(), less); }
template <typename T, typename Less>
void InsertionSort(std::vector<T>& v, Less less) { InsertionSort(v.data(), v.size(), less); }
template <typename T, typename Less>
void MergeSort(std::vector<T>& v, Less less) { MergeSort(v.data(), v.size(), less); }
template <typename T, typename Less>
void QuickSort(std::vector<T>& v, Less less) { QuickSort(v.data(), v.size(), less); }

template <typename T>
void BubbleSort(std::vector<T>& v) { BubbleSort(v.data(), v.size(), DefaultLess()); }
template <typename T>
void InsertionSort(std::vector<T>& v) { InsertionSort(v.data(), v.size(), DefaultLess()); }
template <typename T>
void MergeSort(std::vector<T>& v) { MergeSort(v.data(), v.size(), DefaultLess()); }
template <typename T>
void QuickSort(std::vector<T>& v) { QuickSort(v.data(), v.size(), DefaultLess()); }

#endif
