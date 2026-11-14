#include "circular_queue.h"
#include "stack.h"
#include <cstdlib>
#include <deque>
#include <iostream>
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

struct UndoLike {
    int action = 0;
    long long key = 0;
    string value;
};

void testStackBasics() {
    Stack<int> stack;
    int out = -1;
    check(stack.Empty() && stack.Size() == 0, "new stack is empty");
    check(!stack.Pop(&out) && out == -1, "pop on empty leaves output alone");
    check(!stack.Pop(), "pop on empty with no output");
    check(!stack.Top(&out) && out == -1, "top on empty leaves output alone");

    stack.Push(1);
    stack.Push(2);
    stack.Push(3);
    check(stack.Size() == 3 && !stack.Empty(), "size after pushes");
    check(stack.Top(&out) && out == 3 && stack.Size() == 3, "top does not remove");
    check(stack.Pop(&out) && out == 3, "pop returns the newest");
    check(stack.Pop(&out) && out == 2, "pop returns the next newest");
    check(stack.Pop(nullptr) && stack.Size() == 0, "pop can discard the item");
    check(!stack.Pop(&out), "empty again");
}

void testStackGrowth() {
    Stack<int> stack;
    for (int i = 0; i < 1000; ++i) {
        stack.Push(i);
    }
    check(stack.Size() == 1000, "size after growth");
    int out = 0;
    for (int i = 999; i >= 0; --i) {
        check(stack.Pop(&out) && out == i, "LIFO order after growth");
    }
    check(stack.Empty(), "empty after popping everything");
}

void testStackClearAndReuse() {
    Stack<string> stack;
    stack.Push("a");
    stack.Push("b");
    stack.Clear();
    string out = "unchanged";
    check(stack.Empty() && !stack.Pop(&out) && out == "unchanged", "clear empties");
    stack.Push("again");
    check(stack.Top(&out) && out == "again", "reuse after clear");
}

void testStackWithStructs() {
    Stack<UndoLike> stack;
    for (int i = 0; i < 20; ++i) {
        UndoLike entry;
        entry.action = i % 2;
        entry.key = i * 10;
        entry.value = "value " + to_string(i);
        stack.Push(entry);
    }
    UndoLike out;
    for (int i = 19; i >= 0; --i) {
        check(stack.Pop(&out), "pop struct");
        check(out.action == i % 2 && out.key == i * 10
              && out.value == "value " + to_string(i), "struct fields survive");
    }
}

void testStackRandomAgainstVector() {
    mt19937 rng(99);
    Stack<int> stack;
    vector<int> expected;
    for (int step = 0; step < 5000; ++step) {
        if (rng() % 100 < 55) {
            int value = static_cast<int>(rng() % 1000);
            stack.Push(value);
            expected.push_back(value);
        } else {
            int out = -1;
            bool popped = stack.Pop(&out);
            check(popped == !expected.empty(), "random pop result");
            if (popped) {
                check(out == expected.back(), "random pop value");
                expected.pop_back();
            }
        }
        check(stack.Size() == expected.size(), "random size");
        int top = -1;
        check(stack.Top(&top) == !expected.empty(), "random top result");
        if (!expected.empty()) {
            check(top == expected.back(), "random top value");
        }
    }
}

void testQueueBasics() {
    CircularQueue<int> queue(3);
    int out = -1;
    check(queue.Empty() && queue.Size() == 0 && queue.Capacity() == 3,
          "new queue is empty");
    check(!queue.At(0, &out) && out == -1, "At on empty queue");

    queue.Push(10);
    queue.Push(20);
    check(queue.Size() == 2 && !queue.Full(), "size after two pushes");
    check(queue.At(0, &out) && out == 10, "oldest is At(0)");
    check(queue.At(1, &out) && out == 20, "newest is At(Size - 1)");
    check(!queue.At(2, &out) && out == 20, "index past the end");

    queue.Push(30);
    check(queue.Full() && queue.Size() == 3, "full at capacity");
}

void testQueueOverwritesOldest() {
    CircularQueue<int> queue(3);
    for (int i = 1; i <= 3; ++i) {
        queue.Push(i);
    }
    queue.Push(4);  // drops 1
    int out = 0;
    check(queue.Size() == 3, "size stays at capacity");
    check(queue.At(0, &out) && out == 2, "oldest after one overwrite");
    check(queue.At(1, &out) && out == 3, "middle after one overwrite");
    check(queue.At(2, &out) && out == 4, "newest after one overwrite");

    queue.Push(5);  // drops 2
    queue.Push(6);  // drops 3
    check(queue.At(0, &out) && out == 4, "oldest after wrapping twice");
    check(queue.At(1, &out) && out == 5, "middle after wrapping twice");
    check(queue.At(2, &out) && out == 6, "newest after wrapping twice");
}

void testQueueHistoryLikeUse() {
    CircularQueue<string> history(10);
    for (int i = 1; i <= 25; ++i) {
        history.Push("command " + to_string(i));
    }
    check(history.Size() == 10, "history keeps only 10");
    string line;
    for (size_t i = 0; i < 10; ++i) {
        check(history.At(i, &line) && line == "command " + to_string(16 + i),
              "history is oldest to newest");
    }
}

void testQueueClearAndEdgeCapacities() {
    CircularQueue<int> queue(4);
    queue.Push(1);
    queue.Push(2);
    queue.Clear();
    int out = -1;
    check(queue.Empty() && !queue.At(0, &out), "clear empties");
    queue.Push(7);
    check(queue.At(0, &out) && out == 7 && queue.Size() == 1, "reuse after clear");

    CircularQueue<int> single(1);
    single.Push(1);
    single.Push(2);
    check(single.Size() == 1 && single.At(0, &out) && out == 2,
          "capacity 1 keeps the newest");

    CircularQueue<int> zero(0);
    zero.Push(5);
    zero.Push(6);
    check(zero.Capacity() == 1 && zero.At(0, &out) && out == 6,
          "capacity 0 is treated as 1");
}

void testQueueRandomAgainstDeque() {
    mt19937 rng(2024);
    for (size_t capacity : {1u, 2u, 5u, 10u}) {
        CircularQueue<int> queue(capacity);
        deque<int> expected;
        for (int step = 0; step < 2000; ++step) {
            if (rng() % 100 < 90) {
                int value = static_cast<int>(rng() % 1000);
                queue.Push(value);
                expected.push_back(value);
                if (expected.size() > capacity) {
                    expected.pop_front();
                }
            } else {
                queue.Clear();
                expected.clear();
            }
            check(queue.Size() == expected.size(), "random queue size");
            for (size_t i = 0; i < expected.size(); ++i) {
                int out = -1;
                check(queue.At(i, &out) && out == expected[i],
                      "random queue order");
            }
        }
    }
}

int main() {
    testStackBasics();
    testStackGrowth();
    testStackClearAndReuse();
    testStackWithStructs();
    testStackRandomAgainstVector();
    testQueueBasics();
    testQueueOverwritesOldest();
    testQueueHistoryLikeUse();
    testQueueClearAndEdgeCapacities();
    testQueueRandomAgainstDeque();
    cout << "All Stack and CircularQueue tests passed." << endl;
    return 0;
}
