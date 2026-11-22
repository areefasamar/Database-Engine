#ifndef MINIDB_STACK_H
#define MINIDB_STACK_H

#include <cstddef>
#include <utility>

// Last in, first out. Backed by a raw array that doubles when full.
// Used for UNDO. T must be default constructible and assignable.
template <typename T>
class Stack {
private:
    T* data;
    std::size_t count;
    std::size_t capacity;

    void grow() {
        std::size_t newCapacity = capacity * 2;
        T* bigger = new T[newCapacity];
        for (std::size_t i = 0; i < count; ++i) {
            bigger[i] = std::move(data[i]);
        }
        delete[] data;
        data = bigger;
        capacity = newCapacity;
    }

public:
    Stack() : data(new T[kInitialCapacity]), count(0),
              capacity(kInitialCapacity) {}
    ~Stack() { delete[] data; }

    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    void Push(const T& item) {
        if (count == capacity) {
            grow();
        }
        data[count] = item;
        ++count;
    }

    // Removes the top item. Returns false (and leaves *out alone) if empty.
    // Pass nullptr if you only want to discard the item.
    bool Pop(T* out = nullptr) {
        if (count == 0) {
            return false;
        }
        --count;
        if (out != nullptr) {
            *out = std::move(data[count]);
        }
        data[count] = T();  // release anything the slot still holds
        return true;
    }

    // Copies the top item without removing it. Returns false if empty.
    bool Top(T* out) const {
        if (count == 0) {
            return false;
        }
        if (out != nullptr) {
            *out = data[count - 1];
        }
        return true;
    }

    bool Empty() const { return count == 0; }
    std::size_t Size() const { return count; }

    void Clear() {
        for (std::size_t i = 0; i < count; ++i) {
            data[i] = T();
        }
        count = 0;
    }

private:
    static const std::size_t kInitialCapacity = 8;
};

#endif
