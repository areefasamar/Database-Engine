#include "avl_index.h"
#include "linked_list_index.h"
#include "sorted_array_index.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

using namespace std;

// Checks stay active even if NDEBUG is enabled.
void check(bool condition, const char* message) {
    if (!condition) {
        cerr << "FAILED: " << message << endl;
        exit(1);
    }
}

// Runs the same random operations on all three Index implementations and
// checks that every answer is identical.
void testThreeWayAgreement(unsigned seed, int steps, int keySpan) {
    mt19937 rng(seed);
    LinkedListIndex list;
    SortedArrayIndex array;
    AVLIndex tree;

    for (int step = 0; step < steps; ++step) {
        Key key = static_cast<Key>(rng() % static_cast<unsigned>(keySpan))
                  - keySpan / 2;
        int action = static_cast<int>(rng() % 100);

        if (action < 45) {
            string value = "v" + to_string(rng());
            bool a = list.Put(key, value);
            bool b = array.Put(key, value);
            bool c = tree.Put(key, value);
            check(a == b && b == c, "Put results agree");
        } else if (action < 70) {
            bool a = list.Remove(key);
            bool b = array.Remove(key);
            bool c = tree.Remove(key);
            check(a == b && b == c, "Remove results agree");
        } else if (action < 90) {
            string va = "x", vb = "y", vc = "z";
            bool a = list.Get(key, &va);
            bool b = array.Get(key, &vb);
            bool c = tree.Get(key, &vc);
            check(a == b && b == c, "Get results agree");
            if (a) {
                check(va == vb && vb == vc, "Get values agree");
            }
        } else {
            Key other = static_cast<Key>(rng() % static_cast<unsigned>(keySpan))
                        - keySpan / 2;
            auto a = list.Range(key, other);
            auto b = array.Range(key, other);
            auto c = tree.Range(key, other);
            check(a == b && b == c, "Range results agree");
        }

        check(list.Size() == array.Size() && array.Size() == tree.Size(),
              "Size agrees");
        if (step % 200 == 0) {
            check(tree.IsValid(), "AVL invariants hold");
        }
    }

    Key low = numeric_limits<Key>::min();
    Key high = numeric_limits<Key>::max();
    check(list.Range(low, high) == array.Range(low, high)
          && array.Range(low, high) == tree.Range(low, high),
          "final full range agrees");
    check(tree.IsValid(), "final AVL invariants hold");
}

int main() {
    testThreeWayAgreement(1, 5000, 200);    // many updates and removals
    testThreeWayAgreement(2, 5000, 5000);   // mostly new keys
    testThreeWayAgreement(3, 3000, 20);     // tiny key space, heavy churn
    cout << "All index cross-check tests passed." << endl;
    return 0;
}
