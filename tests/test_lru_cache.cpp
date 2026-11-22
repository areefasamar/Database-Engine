#include "../cache/lru_cache.h"
#include <iostream>
#include <string>
#include <cassert>

int main() {
    // Capacity 3
    LRUCache cache(3);

    cache.Put(1, "A");
    cache.Put(2, "B");
    cache.Put(3, "C");
    assert(cache.Size() == 3);

    std::string out;
    // Access key 1 -> now 1 is MRU, 2 is LRU
    assert(cache.Get(1, &out) == true && out == "A");

    // Put key 4 -> should evict key 2
    cache.Put(4, "D");
    assert(cache.Size() == 3);

    assert(cache.Get(2, &out) == false); // evicted!
    assert(cache.Get(1, &out) == true && out == "A");
    assert(cache.Get(3, &out) == true && out == "C");
    assert(cache.Get(4, &out) == true && out == "D");

    // Test remove
    cache.Remove(3);
    assert(cache.Size() == 2);
    assert(cache.Get(3, &out) == false);

    std::cout << "All LRUCache tests passed!\n";
    return 0;
}
