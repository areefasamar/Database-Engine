#pragma once
// ============================================================================
// persistence.h  -  atomic disk persistence for MiniDB
//
// Formats:
//   Header:  MINIDB 1 \n <count> \n
//   Records: <key> <length> <exact value bytes> \n
//
// Saves to a .tmp file first and renames atomically to prevent corrupt files.
// ============================================================================
#include <string>
#include <vector>
#include "../common.h"

class Persistence {
public:
    static bool Save(const std::string& file, const std::vector<KeyValue>& records);
    static bool Load(const std::string& file, std::vector<KeyValue>* out);
};
