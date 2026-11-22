#include "../cache/hash_table.h"
#include <iostream>
#include <string>
#include <cassert>

int main() {
    HashTable<std::string> ht;

    assert(ht.Size() == 0);

    // Test Put and Get
    assert(ht.Put(1, "one") == true);
    assert(ht.Put(2, "two") == true);
    assert(ht.Put(3, "three") == true);
    assert(ht.Size() == 3);

    std::string val;
    assert(ht.Get(1, &val) == true && val == "one");
    assert(ht.Get(2, &val) == true && val == "two");
    assert(ht.Get(3, &val) == true && val == "three");
    assert(ht.Get(4, &val) == false);

    // Test update
    assert(ht.Put(2, "TWO_UPDATED") == false);
    assert(ht.Get(2, &val) == true && val == "TWO_UPDATED");
    assert(ht.Size() == 3);

    // Test Remove
    assert(ht.Remove(2) == true);
    assert(ht.Get(2, &val) == false);
    assert(ht.Size() == 2);
    assert(ht.Remove(2) == false);

    // Test rehash under heavier load
    for (int i = 10; i < 100; ++i) {
        ht.Put(i, "val_" + std::to_string(i));
    }
    for (int i = 10; i < 100; ++i) {
        assert(ht.Get(i, &val) == true && val == ("val_" + std::to_string(i)));
    }

    std::cout << "All HashTable tests passed!\n";
    return 0;
}
