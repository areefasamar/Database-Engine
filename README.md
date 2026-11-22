# MiniDB: An Embedded Database Engine in C++

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
![DSA Project](https://img.shields.io/badge/course-DSA%20project-lightgrey.svg)

A lightweight, persistent key-value database built from scratch in C++ (C++11/17 standard). It indexes records with an **AVL tree**, speeds up repeated reads with an **LRU cache** (backed by a custom Hash Table and Doubly-Linked List), supports **UNDO** operations via a dynamic stack, maintains command **HISTORY** with a circular queue, and provides stable value sorting with **Merge Sort**. Data is atomically persisted to disk and reloaded on start.

Every data structure and algorithm in MiniDB was custom-implemented for our Data Structures & Algorithms course project. No third-party containers or STL collections (`std::map`, `std::unordered_map`, `std::list`) are used in the core engine.

---

## What It Does

```text
minidb > PUT 101 "Areefa"
  [OK] Inserted  key=101  =>  "Areefa"
minidb > PUT 102 "Ali"
  [OK] Inserted  key=102  =>  "Ali"
minidb > GET 101
  [OK] key=101  =>  "Areefa"
minidb > SCAN 100 200
  [>>] Scanning keys 100 to 200 ...
  +----------------------------------------------------------+
  |  KEY           VALUE
  +----------------------------------------------------------+
  |       101  =>  Areefa
  |       102  =>  Ali
  +----------------------------------------------------------+
  2 record(s) returned.
minidb > DELETE 101
  [OK] Deleted  key=101
minidb > UNDO
  [OK] Last change undone.
minidb > SORT VALUE
  +----------------------------------------------------------+
  |  KEY           VALUE
  +----------------------------------------------------------+
  |       102  =>  Ali
  |       101  =>  Areefa
  +----------------------------------------------------------+
minidb > STATS
  +==========================================================+
  |                  DATABASE  STATISTICS                  |
  +----------------------------------------------------------+
  |  Records stored   : 64
  |  Undo stack depth : 1
  |  LRU Cache usage  : 16/16
  |  Cache Hits       : 2
  |  Cache Misses     : 1
  |  Data file        : mydata.db
  +==========================================================+
minidb > EXIT
```

Keys are integers (`int`). Values are text strings (supports plain tokens or quoted strings with spaces, e.g., `PUT 1 "MINAHIL KHAN"`).

---

## Supported Commands

| Command | Syntax | Operational Description |
|---|---|---|
| **PUT** | `PUT <key> <value>` | Insert a record, or update it if the key exists. Updates both the tree index and LRU cache. |
| **GET** | `GET <key>` | Look up a record. Hits the LRU cache first ($O(1)$); on miss, searches the AVL tree ($O(\log N)$) and warms cache. |
| **DELETE** | `DELETE <key>` | Remove a record from tree, invalidate cache entry, and push restoration delta to Undo stack. |
| **SCAN** | `SCAN <lo> <hi>` | Return all records with keys in range $[lo, hi]$ in sorted key order using tree traversal. |
| **SORT VALUE** | `SORT VALUE` | Display all records sorted alphabetically by string value using custom MergeSort. |
| **UNDO** | `UNDO` | Reverse the last mutating operation (`PUT` or `DELETE`), restoring previous state. |
| **HISTORY** | `HISTORY` | Display the last 10 commands executed using a fixed-capacity circular queue. |
| **STATS** | `STATS` | Display stored record count, active undo depth, LRU cache usage & hit/miss ratio, and file path. |
| **SAVE** | `SAVE` | Atomically persist all records to `mydata.db` immediately. |
| **HELP** | `HELP` | Display the interactive command reference card. |
| **EXIT** | `EXIT` | Automatically persist records to disk and safely exit. |

---

## System Architecture

```text
User (CLI / Dev-C++ / Terminal)
   |
   v
[ Parser ]  text -> Command object
   |        HISTORY (CircularQueue)
   v
[ Database ]  Transaction coordinator & Undo Stack
   |
   +--------------------+
   |                    |
   v                    v
[ LRU Cache ]      [ Index (ADT Interface) ]
HashTable +         |-- AVLIndex          (Primary: self-balancing BST)
Doubly-Linked List  |-- SortedArrayIndex  (Baseline: binary search)
   |                |-- LinkedListIndex   (Baseline: linear search)
   |                    |
   +----------+---------+
              v
      [ Persistence ]  <->  mydata.db (Atomic 2-phase disk format: MINIDB 1)

[ Sorting ]  MergeSort, QuickSort, InsertionSort, BubbleSort (used by SORT VALUE)
```

The `Database` communicates strictly through the `Index` abstract interface, allowing the concrete storage structure (`AVLIndex`, `SortedArrayIndex`, or `LinkedListIndex`) to be selected at startup with zero changes to business logic.

---

## Data Structures & Algorithms Used

| Data Structure / Algorithm | File Location | Purpose & Complexity |
|---|---|---|
| **AVL Tree (Self-Balancing BST)** | `index/avl_index.*` | Primary search index. Balance factor $\in \{-1, 0, 1\}$. Insert, search, delete: $O(\log N)$. Ordered range scan: $O(\log N + k)$. |
| **Hash Table (Chaining + Rehashing)** | `cache/hash_table.h` | Prime bucket array with singly-linked collision chains and dynamic rehashing at load $> 0.75$. Lookup, insert, delete: $O(1)$ avg. |
| **LRU Cache** | `cache/lru_cache.*` | Combines `HashTable<Node*>` with a doubly-linked list. Fast read hits: $O(1)$. LRU eviction: $O(1)$. |
| **Dynamic Array Stack** | `data_structures/stack.h` | Array-backed LIFO stack that doubles on full. Powers reversible transaction `UNDO`. Push/Pop: $O(1)$. |
| **Circular Queue (Ring Buffer)** | `data_structures/circular_queue.h` | Fixed rolling history of last 10 commands without memory reallocation. Push/Access: $O(1)$. |
| **Merge Sort** | `data_structures/sorting.h` | Stable, divide-and-conquer sorting algorithm used by `SORT VALUE`. Time: $O(N \log N)$. |
| **Quick Sort, Insertion, Bubble** | `data_structures/sorting.h` | Benchmark comparison algorithms. |
| **Singly Linked List Index** | `index/linked_list_index.*` | Baseline comparison index (Level 1). Linear search: $O(N)$. |
| **Sorted Dynamic Array Index** | `index/sorted_array_index.*` | Baseline comparison index (Level 2). Binary search: $O(\log N)$, Insert/Delete: $O(N)$. |
| **Atomic Persistence** | `database/persistence.*` | Length-prefixed binary/text format (`MINIDB 1`), temporary `.tmp` swap to prevent partial writes. |

---

## Project Structure

```text
Database-Engine/
├── common.h                     <- Shared primitive types (Key, KeyValue)
├── index.h                      <- Abstract Index interface
├── main.cpp                     <- Application entry point (initializes index, DB, CLI)
├── mydata.db                    <- Persistent storage file (64 pre-seeded records)
├── PROJECT_REPORT.md            <- Complete project report & technical specification
│
├── cache/                       <- Arbish's module
│   ├── hash_table.h             <- Templated separate-chaining Hash Table
│   ├── lru_cache.h              <- LRU Cache header
│   └── lru_cache.cpp            <- LRU Cache implementation (Hash Table + Doubly-Linked List)
│
├── data_structures/             <- Core ADTs & Algorithms
│   ├── stack.h                  <- Dynamic array Stack (UNDO)
│   ├── circular_queue.h         <- Ring-buffer Circular Queue (HISTORY)
│   └── sorting.h                <- Bubble, Insertion, Merge, and Quick Sort
│
├── index/                       <- Search tree implementations
│   ├── avl_index.h              <- AVL tree header with 4 rotation cases
│   ├── avl_index.cpp            <- Self-balancing AVL tree implementation
│   ├── linked_list_index.h/.cpp <- Baseline unsorted singly-linked list
│   └── sorted_array_index.h/.cpp<- Baseline sorted dynamic array with binary search
│
├── parser/                      <- Command processor
│   ├── parser.h                 <- Token and Command structs
│   └── parser.cpp               <- Quoted tokenizer & syntax validator
│
├── database/                    <- Core coordinator & UI
│   ├── database.h               <- Database coordinator header
│   ├── database.cpp             <- CRUD, Undo, Cache, & Persistence coordination
│   ├── cli.h                    <- REPL interface declaration
│   ├── cli.cpp                  <- ANSI-colored terminal interface
│   ├── persistence.h            <- Persistence class declaration
│   └── persistence.cpp          <- Crash-safe disk save/load implementation
│
└── tests/                       <- Standalone test suites (one main() per test)
    ├── test_avl.cpp             <- 100,000-operation AVL tree invariant test
    ├── test_hash_table.cpp      <- Hash Table collision & rehash test
    ├── test_lru_cache.cpp       <- LRU Cache eviction & lookup test
    ├── test_persistence.cpp     <- Atomic serialization & reload test
    ├── test_cli.cpp             <- REPL command tests
    ├── test_database.cpp        <- Integration & rollback tests
    ├── test_index_crosscheck.cpp<- Cross-checks AVL vs Array vs List
    ├── test_linked_list_index.cpp
    ├── test_sorted_array_index.cpp
    ├── test_sorting.cpp         <- Sorting algorithm correctness tests
    └── test_stack_queue.cpp     <- Stack & Circular Queue unit tests
```

---

## How to Build & Run

### In Embarcadero Dev-C++ (Recommended for Submission)
1. Open Dev-C++ and open your project (or create a new empty project).
2. Add the following **9 source files** to the project:
   - `main.cpp`
   - `cache/lru_cache.cpp`
   - `index/avl_index.cpp`
   - `index/linked_list_index.cpp`
   - `index/sorted_array_index.cpp`
   - `parser/parser.cpp`
   - `database/database.cpp`
   - `database/cli.cpp`
   - `database/persistence.cpp`
3. Set the C++ standard to C++11 or C++17:
   `Tools` -> `Compiler Options` -> Add `-std=c++11` or `-std=c++17` in the compiler parameters.
4. Press **F12** (Rebuild All) then **F11** (Compile & Run).

### Command Line (GCC / Clang)
```bash
g++ -std=c++17 -O2 main.cpp cache/lru_cache.cpp index/avl_index.cpp index/linked_list_index.cpp index/sorted_array_index.cpp parser/parser.cpp database/database.cpp database/cli.cpp database/persistence.cpp -o minidb.exe
./minidb.exe
```

### Running Tests
Each test file under `tests/` is a self-contained unit test executable with its own `main()`:
```bash
g++ -std=c++17 tests/test_avl.cpp index/avl_index.cpp -o test_avl.exe && ./test_avl.exe
g++ -std=c++17 tests/test_hash_table.cpp -o test_hash_table.exe && ./test_hash_table.exe
g++ -std=c++17 tests/test_lru_cache.cpp cache/lru_cache.cpp -o test_lru_cache.exe && ./test_lru_cache.exe
g++ -std=c++17 tests/test_persistence.cpp database/persistence.cpp -o test_persistence.exe && ./test_persistence.exe
```

---

## Team Allocation & Responsibilities

| Member | Assigned Modules | Deliverables |
|---|---|---|
| **Areefa Samar (Lead)** | System Integration, Baselines, Data Structures, & CLI | Project architecture, `Index` ADT, `LinkedListIndex`, `SortedArrayIndex`, `Stack` (UNDO), `CircularQueue` (HISTORY), 4 sorting algorithms, CLI REPL, command tokenizer & parser, and project integration. |
| **Areeba (CT-25070)** | Index Layer (AVL Tree) | `AVLIndex` implementation: 4 tree rotation cases (LL, RR, LR, RL), in-order successor deletion, height bookkeeping, range query pruning, and invariant tree validation (`IsValid()`). |
| **Arbish** | Cache & Persistence Layer | Templated `HashTable` with separate chaining and prime rehashing, $O(1)$ `LRUCache` combining hash table and doubly-linked list, and atomic 2-phase disk serialization (`Persistence`). |

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
