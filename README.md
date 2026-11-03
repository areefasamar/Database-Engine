# MiniDB: An Embedded Database Engine in C++

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Build: CMake](https://img.shields.io/badge/build-CMake-064F8C.svg)
![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
![DSA Project](https://img.shields.io/badge/course-DSA%20project-lightgrey.svg)

A small persistent key-value database built from scratch in C++17. It finds records with an AVL tree index, speeds up repeated reads with an LRU cache, and supports undo. Data is saved to a file and reloaded on start.

Our Data Structures and Algorithms end-semester project. Every data structure is written by us. No external database libraries.

---

## What it does

```
> PUT 101 "Areefa"
SUCCESS: Record 101 inserted.
> PUT 102 "Ali"
SUCCESS: Record 102 inserted.
> GET 101
101 -> Areefa
> SCAN 100 200
101 -> Areefa
102 -> Ali
> DELETE 101
SUCCESS: Record 101 deleted.
> UNDO
SUCCESS: Undid DELETE 101.
> SORT VALUE
102 -> Ali
101 -> Areefa
> EXIT
```

Keys are integers. Values are text (a word or a "quoted string").

## Commands

| Command | What it does |
|---|---|
| `PUT <key> <value>` | Insert a record, or update it if the key exists |
| `GET <key>` | Fetch one record |
| `DELETE <key>` | Remove a record |
| `SCAN <lo> <hi>` | Return all records with keys from lo to hi, in order |
| `SORT VALUE` | Show all records sorted by value |
| `UNDO` | Reverse the last PUT or DELETE |
| `HISTORY` | Show the last 10 commands |
| `STATS` | Show cache hit/miss ratio, record count, tree height |
| `EXIT` | Save everything to the file and quit |

---

## How it works

The database keeps records in an AVL tree, which stays balanced so every lookup takes O(log N) steps. A small LRU cache sits in front of it so recent reads are answered in O(1). Records are saved to a plain file on exit and loaded again on start.

### Architecture

```
User (CLI or C++ API)
   |
   v
[ Parser ]  text -> Command
   |        HISTORY (circular queue)
   v
[ Database ]  decides what to do, keeps the UNDO stack
   |
   +--------------------+
   |                    |
   v                    v
[ LRU Cache ]      [ Index (ADT) ]
hash table +        |-- AVLIndex          (final choice)
doubly linked list  |-- SortedArrayIndex  (baseline, binary search)
   |                |-- LinkedListIndex   (baseline, linear search)
   |                    |
   +----------+---------+
              v
      [ Persistence ]  <->  mydata.db (plain file)

[ Sorting ]  merge sort, quick sort (used by SORT)
```

The Database talks only to the `Index` interface, so the index implementation can be swapped by changing one line. We built the two simple indexes first, so the system worked before the AVL tree was ready, and we use them as benchmark baselines.

### What happens on `PUT 101 "Areefa"`

1. Parser turns the text into a command and adds it to HISTORY.
2. Database reads the old value of key 101 (needed for UNDO).
3. The AVL tree inserts or updates the key and rebalances with rotations if needed.
4. The cache stores the record so the next GET is fast.
5. The opposite action is pushed on the UNDO stack.

`GET` checks the cache first (hit: O(1)). On a miss it searches the tree, then stores the result in the cache.

### Data structures used

| Data structure / algorithm | Where it is used |
|---|---|
| AVL tree | Main index: search, insert, delete, ordered range scan |
| Binary search tree | Built first, then upgraded to AVL |
| Hash table (chaining, rehashing) | Inside the LRU cache |
| Doubly linked list | LRU ordering, O(1) move and evict |
| Singly linked list | Hash bucket chains, LinkedListIndex |
| Stack | UNDO |
| Circular queue | HISTORY |
| Linear search, binary search | Baseline indexes and benchmarks |
| Merge sort, quick sort | SORT command |
| Recursion | AVL operations, traversals, sorting |

### AVL tree

- Each node stores key, value, height, left and right child.
- Balance factor = height(left) - height(right). If it reaches 2 or -2, we rotate.
- Four cases: Left-Left, Right-Right, Left-Right, Right-Left.
- Insert and delete rebalance every node on the path back to the root.
- Range scan is a recursive in-order walk that skips subtrees outside the range.
- Sorted input does not slow it down. A plain BST would turn into a chain.

### LRU cache

- Our own hash table maps key to a node in a doubly linked list.
- Head of the list is most recently used. Tail is least recently used.
- Get moves the node to the head. When the cache is full, the tail is evicted.
- DELETE and UNDO also update the cache, so it never returns stale data.

### Persistence

Records are saved to a plain text file, one after another, with the value length before the value so spaces and line breaks are safe. The file is written to a temporary file first and then renamed, so a failure in the middle does not destroy the old file. On start, records are loaded back into the tree.

---

## Project structure

```
Database-Engine/
  CMakeLists.txt
  README.md
  LICENSE
  include/
    common.h
    index.h
    linked_list_index.h
    sorted_array_index.h
    avl_index.h
    hash_table.h
    lru_cache.h
    stack.h
    circular_queue.h
    sorting.h
    persistence.h
    parser.h
    database.h
  src/
    main.cpp            (CLI)
    ...                 (one .cpp per module that needs one)
  tests/
    test_linked_list_index.cpp
    test_sorted_array_index.cpp
    test_avl.cpp
    test_hash_table.cpp
    test_lru_cache.cpp
    test_stack_queue.cpp
    test_sorting.cpp
    test_parser.cpp
    test_persistence.cpp
    test_database.cpp
  bench/
    bench_main.cpp
  docs/
    architecture.md
```

---

## Build and run

Requirements: C++17 compiler (g++ or clang), CMake.

```bash
git clone <repo-url>
cd Database-Engine
mkdir build && cd build
cmake ..
cmake --build .
```

Run the CLI:

```bash
./minidb mydata.db
```

Run the tests:

```bash
ctest --output-on-failure
```

Run with sanitizers (catches leaks and invalid memory access):

```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" ..
cmake --build .
```

## Using it as a C++ library

```cpp
#include "database.h"

Database db("mydata.db");
db.Put(101, "Areefa");

std::string value;
if (db.Get(101, &value)) {
    // value == "Areefa"
}

auto rows = db.Scan(100, 200);   // sorted by key
db.Delete(101);
db.Undo();                       // 101 is back
// destructor saves everything to the file
```

---

## Testing

| Module | What is tested |
|---|---|
| Linked list and sorted array index | Put, update, get, remove, range, same answers as each other |
| AVL tree | Sequential, reverse and random inserts, random deletes, validity check after every step, height bound, sorted range scan |
| Hash table | Collisions, update, remove from a chain, rehash keeps every key |
| LRU cache | Eviction order, update moves to front, remove, hit/miss counts |
| Stack and circular queue | Push, pop, empty cases, wrap-around, overwrite when full |
| Sorting | Empty, duplicates, sorted, reverse, random, all algorithms agree |
| Parser | Valid commands, quoted strings, missing arguments, non-numeric keys |
| Persistence | Put 1000 records, close, reopen, all still equal |
| Database | UNDO restores state, cache never returns a deleted key |
| Stress | 100k random operations compared with a reference |

---

## Benchmarks

Measured with `std::chrono`. Machine: `<CPU, RAM, OS>`. Compiler flags: `-O2`.

> Numbers below are filled in after running `bench/bench_main.cpp`. Do not trust a row until it has a value.

**1. GET latency by index type**

| N keys | Linked list (linear) | Sorted array (binary search) | AVL tree |
|---|---|---|---|
| 1,000 | TBD | TBD | TBD |
| 10,000 | TBD | TBD | TBD |
| 100,000 | TBD | TBD | TBD |

**2. Sorted inserts: plain BST vs AVL**

| N keys | BST height | AVL height | BST time | AVL time |
|---|---|---|---|---|
| 1,000 | TBD | TBD | TBD | TBD |
| 10,000 | TBD | TBD | TBD | TBD |

**3. Cache size vs hit rate**

| Cache size | Hit rate |
|---|---|
| 4 | TBD |
| 16 | TBD |
| 64 | TBD |
| 256 | TBD |

**4. Sorting time**

| Algorithm | N = 1,000 | N = 10,000 |
|---|---|---|
| Bubble sort | TBD | TBD |
| Insertion sort | TBD | TBD |
| Merge sort | TBD | TBD |
| Quick sort | TBD | TBD |

---

## Complexity

| Operation | Time |
|---|---|
| PUT | O(log N) |
| GET | O(1) on a cache hit, O(log N) on a miss |
| DELETE | O(log N) |
| SCAN (k results) | O(log N + k) |
| UNDO | O(log N) |
| SORT VALUE | O(N log N) |
| Cache get / put | O(1) |

---

## Design decisions

- **Why an AVL tree and not a plain BST?** A BST turns into a chain when keys arrive in order, and sequential IDs are common. AVL keeps the height near 1.44 x log2(N).
- **Why keep a tree if a hash table is faster?** A hash table cannot return keys in order, so SCAN and sorted output need the tree.
- **Why a hash table plus doubly linked list for LRU?** The hash table finds the node, the list moves and evicts it, both in O(1).
- **Why a stack for UNDO?** Last change is the first to be reversed.
- **Why a circular queue for HISTORY?** Keeps only the last 10 commands without shifting anything.
- **Why build two baseline indexes?** They give us a working system early, a benchmark comparison, and a way to test the AVL tree against known-correct answers.
- **Why build in levels?** So something always runs. Each level is a working demo.

| Level | What it adds |
|---|---|
| 1 | Linked-list index, Database class, CLI, save and load |
| 2 | Sorted-array index, UNDO, HISTORY |
| 3 | AVL tree replaces the baseline index |
| 4 | Hash table and LRU cache, SCAN, SORT, STATS |
| 5 | Tests, benchmarks, README, stretch goals |

## Known limitations

- Single-threaded. No locks.
- Records are saved on EXIT. If the program crashes before saving, changes since the last save are lost.
- Everything is held in memory while the program runs, so the data set must fit in RAM.
- Keys are 64-bit integers only.
- Not a SQL database. No joins, no query optimizer, no network layer.

## Future work

Append-only log for crash safety, BATCH command from a file (queue), key expiry with a priority queue (heap), graph links between records, more index types.

---

## Team

| Member | Owns |
|---|---|
| Areeba | AVL tree: insert, delete, rotations, range scan, validity checks |
| Arbish | Hash table, LRU cache, persistence, CMake setup |
| Areefa | Index interface, linked-list and sorted-array indexes, stack, circular queue, sorting, parser and CLI, Database class, integration, benchmarks, README |

Contribution history is in the git log. Each person works on their own branch and merges through a pull request.

## Learning resources

- Our course lectures and slides
- USFCA data structure visualizations (AVL trees, hash tables)

Used to understand concepts. Code is our own.

## License

MIT. See the [LICENSE](LICENSE) file.

Copyright (c) 2026 Areefa, Areeba, Arbish
(hello, its arbish here)
