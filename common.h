#ifndef MINIDB_COMMON_H
#define MINIDB_COMMON_H

// Shared primitive types used across the entire engine.
// Every other header that needs Key or KeyValue includes this file.

#include <string>
#include <utility>

// Keys are plain integers. Change here to change the whole engine.
using Key = int;

// A single record: (key, value) pair stored in the index.
using KeyValue = std::pair<Key, std::string>;

#endif  // MINIDB_COMMON_H
