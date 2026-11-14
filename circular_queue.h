#ifndef MINIDB_CIRCULAR_QUEUE_H
#define MINIDB_CIRCULAR_QUEUE_H

#include <cstddef>

// Fixed-capacity queue whose end wraps around to the start.
// Pushing into a full queue overwrites the OLDEST item.
// Used for HISTORY (keeps the last N commands).
// T must be default constructible and assignable.
template <typename T>
class CircularQueue {
private:
    T* data;
    std::size_t capacity;
    std::size_t head;   // index of the oldest item
    std::size_t count;

public:
    // A capacity of 0 is treated as 1 so the queue is always usable.
    explicit CircularQueue(std::size_t maxItems)
        : data(nullptr), capacity(maxItems == 0 ? 1 : maxItems),
          head(0), count(0) {
        data = new T[capacity];
    }
    ~CircularQueue() { delete[] data; }

    CircularQueue(const CircularQueue&) = delete;
    CircularQueue& operator=(const CircularQueue&) = delete;

    void Push(const T& item) {
        std::size_t tail = (head + count) % capacity;
        data[tail] = item;
        if (count == capacity) {
            head = (head + 1) % capacity;  // full: the oldest item is dropped
        } else {
            ++count;
        }
    }

    // At(0) is the oldest item, At(Size() - 1) is the newest.
    // Returns false (and leaves *out alone) if index is out of range.
    bool At(std::size_t index, T* out) const {
        if (index >= count) {
            return false;
        }
        if (out != nullptr) {
            *out = data[(head + index) % capacity];
        }
        return true;
    }

    bool Empty() const { return count == 0; }
    bool Full() const { return count == capacity; }
    std::size_t Size() const { return count; }
    std::size_t Capacity() const { return capacity; }

    void Clear() {
        for (std::size_t i = 0; i < capacity; ++i) {
            data[i] = T();
        }
        head = 0;
        count = 0;
    }
};

#endif
