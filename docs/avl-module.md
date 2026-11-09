# AVL module

`include/avl_index.h` declares the index; `src/avl_index.cpp` implements it.
The module uses the shared `Key`, `KeyValue`, and `Index` types. It does not
perform file I/O or manage the cache.

## Behavior

- `Put` returns true for a new key and false when replacing an existing value.
- `Get` returns false for a missing key and leaves the output string unchanged.
  A null output pointer checks existence without copying the value.
- `Remove` returns true only when a record was removed.
- `Range` includes both endpoints, returns records in ascending key order,
  and returns an empty vector when the lower bound exceeds the upper bound.
- `Size` returns the number of distinct keys.
- `Height` uses empty = 0 and leaf = 1.
- `IsValid` independently checks strict BST ordering, balance, stored heights,
  and the record count.
- Copying is disabled. Destruction frees children before their parent.

Insertion and deletion rebalance as recursion returns. LL and RR need one
rotation; LR and RL first rotate the child, then the original unbalanced node.
Each rotation updates the lower node's height before the new subtree root's.
Two-child deletion copies the successor's key and value and removes its old node.

## Complexity

| Operation | Time |
|---|---|
| Put, Get, Remove | O(log N) |
| Range with k returned records | O(log N + k) |
| Size, Height | O(1) |
| IsValid, destruction | O(N) |

String copying additionally depends on the value length. Range allocates space
for its results; recursive tree operations use O(log N) stack space.

## Run the standalone tests

From the repository root in PowerShell, compile the executable outside the repo:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I include src/avl_index.cpp tests/test_avl.cpp -o "$env:TEMP/minidb-test-avl.exe"
& "$env:TEMP/minidb-test-avl.exe"
```

Expected output:

```text
All AVL tests passed (including 100000 mixed operations).
```

The tests cover empty trees, duplicate updates, absent keys, null output,
empty/multiline values, both integer limits, inclusive and reversed ranges,
four insertion rotation cases and transferred subtrees, deletion cases,
balanced-child deletion rotations, and sorted/reverse/random insertion of
1,000 keys. They check the height bound of 15, find every inserted record,
delete in random order, and check invariants after each mutation.
Another 100,000 seeded mixed operations are checked against `std::map`, used
only in the tests. Checks remain enabled with `NDEBUG`.

## Validation and integration status

The standalone suite passed with MSYS2 g++ 15.2.0 using the flags above.
AddressSanitizer/UndefinedBehaviorSanitizer could not link in the installed
Windows toolchain because `libasan` and `libubsan` are unavailable. Memory
sanitizer verification remains pending on a supported toolchain.

The team's `SortedArrayIndex`, Database, and shared build configuration are
not present in this checkout yet. Compare the same operations against that
baseline and add this suite to the team's build when those files are available.
Do not run `tests/test_avl.cpp` as part of the CLI executable: it has its own main.
