# Database-Engine

A small embedded database engine written in C++17.

It stores key-value data in a single file on disk. Close the program, open it again, the data is still there.

Built from scratch as our Data Structures and Algorithms end-semester project. No external database libraries.

---

## What it does

```
> PUT 101 "Areefa"
SUCCESS: Record 101 inserted.
> GET 101
101 -> Areefa
> SCAN 100 200
101 -> Areefa
> DELETE 101
SUCCESS: Record 101 deleted.
> GET 101
NOT FOUND
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
| `STATS` | Show buffer pool hit/miss ratio, page count, tree height |
| `EXIT` | Flush everything to disk and quit |

---

## How it works

Disk is slow, RAM is fast. The whole design is about touching the disk as little as possible.

Three ideas do most of the work:

1. **Pages.** The `.db` file is cut into fixed 4096-byte blocks. Page N lives at byte `N * 4096`. All reads and writes happen one page at a time.
2. **B+ Tree index.** Maps `key -> RecordId{page_id, slot}`. Lookups take O(log N) page reads instead of scanning every record. Leaf nodes are linked left to right, so range scans are fast.
3. **LRU buffer pool.** A fixed number of page-sized frames in RAM. If a page is already there it is a hit. If not, it is loaded from disk, and the least recently used unpinned page is evicted.

### Architecture

```
User (CLI or C++ API)
        |
        v
  [ Parser ]                 text command -> Command object
        |
        v
  [ Database ]               decides what to do
        |
   +----+------------------+
   |                       |
   v                       v
[ B+ Tree ]          [ Record Manager ]
key -> RecordId      slotted pages, serialization
   |                       |
   +-----------+-----------+
               v
     [ Buffer Pool Manager ]   LRU cache of pages in RAM
               |
               v
        [ Disk Manager ]       reads/writes 4KB pages
               |
               v
         mydata.db             single file on disk
```

Both the data pages and the B+ Tree nodes are stored as pages and accessed only through the buffer pool. So the index is persistent and cached too.

### File layout

| Page ID | Content |
|---|---|
| 0 | Meta page: magic number, page count, B+ Tree root page ID, next free page |
| 1, 2, 3 ... | Data pages (slotted records) or B+ Tree node pages |

### What happens on `PUT 101 "Areefa"`

1. Parser turns the text into `Command{PUT, 101, "Areefa"}`.
2. Database checks the index: does key 101 exist? (insert vs update)
3. Record Manager serializes the value and finds a data page with free space.
4. Buffer pool returns the page (hit) or loads it from disk (miss), pinned.
5. The slotted page stores the bytes and returns `RecordId{page 4, slot 2}`. Page is unpinned and marked dirty.
6. B+ Tree inserts `101 -> {4, 2}`. If a leaf overflows it splits, and the split can travel up to the root.

`GET` is the reverse: tree finds the RecordId, buffer pool fetches the page, slotted page returns the bytes.

### Slotted page

Variable-length records are packed into one 4KB page like this:

```
| Header | Slot0 | Slot1 | ... free space ... | Rec1 | Rec0 |
```

- Slot directory grows forward from the start.
- Record bytes grow backward from the end.
- Free space is the gap in the middle.
- Each slot stores `offset` and `length`. Deleting a record sets `length = 0`, and that slot can be reused later.

### Buffer pool

- `unordered_map<PageId, FrameId>` for O(1) lookup.
- Doubly linked list for LRU order, with stored iterators for O(1) move and remove.
- **Pin count:** a page in use is never evicted.
- **Dirty flag:** a modified page is written to disk before it is evicted.
- `FlushAll()` runs on shutdown so no data is lost.

### B+ Tree

- One node = one page = one disk read.
- Internal nodes hold keys and child page IDs. Leaves hold keys and RecordIds.
- Leaf split **copies up** the first key of the new leaf. Internal split **pushes up** the middle key.
- If the root splits, a new root is created. This is the only way the tree grows taller.
- `MAX_KEYS` is a small constant in tests so splits happen with a few inserts.

---

## Project structure

```
minidb/
  CMakeLists.txt
  README.md
  include/
    common.h
    disk_manager.h
    buffer_pool.h
    bplus_tree.h
    record_page.h
    parser.h
    database.h
  src/
    disk_manager.cpp
    buffer_pool.cpp
    bplus_tree.cpp
    record_page.cpp
    parser.cpp
    database.cpp
    main.cpp
  tests/
    test_disk.cpp
    test_buffer_pool.cpp
    test_bplus_tree.cpp
    test_record_page.cpp
    test_parser.cpp
    test_persistence.cpp
  bench/
    bench_main.cpp
  docs/
    architecture.md
```

---

## Build and run

Requirements: C++17 compiler (g++ or clang), CMake 3.14+.

```bash
git clone <repo-url>
cd minidb
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
// destructor flushes everything to disk
```

---

## Testing

| Module | What is tested |
|---|---|
| Disk Manager | Write a page, reopen the file, read back identical bytes |
| Buffer Pool | LRU order, dirty page written on eviction, pinned page never evicted, hit/miss counts |
| Slotted page | Insert until full, get, delete, reuse of deleted slot, variable lengths |
| B+ Tree | Sequential, reverse and random inserts, splits at small `MAX_KEYS`, sorted range scan, compared against `std::map` |
| Parser | Valid commands, quoted strings, missing arguments, non-numeric keys |
| Persistence | Put 1000 records, close, reopen, all still readable |
| Stress | 100k random operations compared against a reference `std::map` |

---

## Benchmarks

Measured with `std::chrono`. Machine: `<CPU, RAM, OS>`. Compiler flags: `-O2`.

> Numbers below are filled in after running `bench/bench_main.cpp`. Do not trust a row until it has a value.

**1. Index vs linear scan (GET latency)**

| N keys | B+ Tree | Linear scan | Speedup |
|---|---|---|---|
| 1,000 | TBD | TBD | TBD |
| 10,000 | TBD | TBD | TBD |
| 100,000 | TBD | TBD | TBD |
| 1,000,000 | TBD | TBD | TBD |

**2. Buffer pool size vs speed**

| Pool size (frames) | Hit rate | Total time |
|---|---|---|
| 4 | TBD | TBD |
| 16 | TBD | TBD |
| 64 | TBD | TBD |
| 256 | TBD | TBD |

**3. Insert throughput**

| Key order | Inserts per second |
|---|---|
| Sequential | TBD |
| Random | TBD |

**4. Range scan**

| Method (1000 keys) | Time |
|---|---|
| Leaf-link `SCAN` | TBD |
| Repeated `GET` | TBD |

---

## Complexity

| Operation | Time |
|---|---|
| PUT | O(log N) page accesses |
| GET | O(log N) page accesses |
| DELETE | O(log N) page accesses |
| SCAN (k results) | O(log N + k) |
| Buffer pool fetch / evict | O(1) |

---

## Design decisions

- **Why a B+ Tree and not a BST or AVL?** High fanout means a very shallow tree (about 3 levels for millions of keys), so few disk reads. Data sits only in linked leaves, which makes range scans cheap.
- **Why 4KB pages?** Matches the common OS and disk block size, so I/O is efficient.
- **Why LRU?** Simple, O(1) with a hash map plus doubly linked list, and works well for typical access patterns.
- **Why store tree nodes as pages?** The index becomes persistent and goes through the same cache as data.
- **Why build in levels?** So something always runs. Each level is a working demo.

| Level | What it adds |
|---|---|
| 1 | Disk manager, slotted pages, `std::map` as index, CLI |
| 2 | Buffer pool with LRU, pin counts, dirty flush |
| 3 | Own B+ Tree in RAM replaces `std::map` |
| 4 | B+ Tree stored in pages, persistence, `SCAN` |
| 5 | Tests, benchmarks, delete with merge, WAL |

## Current status

Update this as things get done.

- [ ] Disk Manager
- [ ] Buffer Pool (LRU, pin, dirty flush)
- [ ] Slotted page and serialization
- [ ] Parser and CLI
- [ ] Database class
- [ ] B+ Tree insert, search, splits
- [ ] B+ Tree range scan
- [ ] B+ Tree stored in pages
- [ ] Persistence test passing
- [ ] 100k stress test passing
- [ ] Benchmarks
- [ ] B+ Tree delete with merge/redistribute (stretch)
- [ ] Write-Ahead Log (stretch)

## Known limitations

- Single-threaded. No locks.
- No crash recovery. A crash during a write can corrupt the file. A Write-Ahead Log would fix this.
- Keys are 64-bit integers only.
- Not a SQL database. No joins, no query optimizer, no network layer.
- Deleted record space inside a page is reused by slot, but pages are not compacted.

## Future work

Write-Ahead Log and crash recovery, B+ Tree delete with merge, typed object API, free-space map for data pages, concurrency, simple SQL-like syntax.

---

## Team

| Member | Owns |
|---|---|
| Areeba | B+ Tree: insert, search, splits, range scan |
| Arbish | Disk Manager, Page, Buffer Pool (LRU, pin counts, dirty flush) |
| Areefa | Slotted page and serialization, Database class, integration, tests, benchmarks, README |

Contribution history is in the git log. Each person works on their own branch and merges through a pull request.

## Learning resources

- CMU 15-445 Database Systems (buffer pool and B+ Tree lectures)
- USFCA B+ Tree visualizer
- *Database Internals* by Alex Petrov
- SQLite file format documentation

Used to understand concepts. Code is our own.

## License

MIT. See the [LICENSE](LICENSE) file.

Copyright (c) 2026 Areefa, Areeba, Arbish
