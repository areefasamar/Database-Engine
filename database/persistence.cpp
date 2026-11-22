// ============================================================================
// persistence.cpp  -  implementation of Persistence (see persistence.h)
// ============================================================================
#include "persistence.h"

#include <cstdio>       // std::remove, std::rename
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
// Helper used by Load: report a corrupt file and give back an empty result.
bool Fail(std::vector<KeyValue>* out, const char* why) {
    out->clear();
    std::cerr << "ERROR: data file is corrupt (" << why << "). Starting empty.\n";
    return false;
}
}  // namespace

bool Persistence::Save(const std::string& file, const std::vector<KeyValue>& records) {
    // Step 1: write everything to a TEMPORARY file first.
    const std::string tmp = file + ".tmp";
    {
        // binary mode: no newline translation, bytes are written exactly as they are.
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;

        out << "MINIDB 1\n" << records.size() << '\n';
        for (const KeyValue& kv : records) {
            // <key> <length> <exact value bytes> newline
            out << kv.first << ' ' << kv.second.size() << ' ';
            out.write(kv.second.data(), static_cast<std::streamsize>(kv.second.size()));
            out << '\n';
        }

        out.flush();
        if (!out) {  // some write failed (disk full, etc.)
            out.close();
            std::remove(tmp.c_str());  // do not leave a half-written temp file
            return false;
        }
    }  // the file is closed here (scope ends)

    // Step 2: rename tmp over the real file. Rename is one step, so we end up
    // with either the complete new file or the complete old file, never half.
    // (std::rename is plain C++, it works with old compilers too.)
    if (std::rename(tmp.c_str(), file.c_str()) != 0) {
        // On Windows, rename fails if the target already exists:
        // delete the old file and try again.
        std::remove(file.c_str());
        if (std::rename(tmp.c_str(), file.c_str()) != 0) {
            std::remove(tmp.c_str());
            return false;
        }
    }
    return true;
}

bool Persistence::Load(const std::string& file, std::vector<KeyValue>* out) {
    out->clear();

    std::ifstream in(file, std::ios::binary);
    if (!in) return true;  // file does not exist: first run, start empty

    // Remember the file size: a record can never be longer than the whole file.
    // (Protects us from a corrupt length like 99999999999.)
    in.seekg(0, std::ios::end);
    const unsigned long long total = static_cast<unsigned long long>(in.tellg());
    in.seekg(0, std::ios::beg);

    // ---- Header: "MINIDB 1" newline, record count, newline -----------------
    std::string magic;
    int version = 0;
    unsigned long long count = 0;
    char c = 0;
    if (!(in >> magic >> version >> count) || magic != "MINIDB" || version != 1) {
        // Fallback: Check if file is legacy tab-separated format
        in.clear();
        in.seekg(0, std::ios::beg);
        std::string line;
        bool foundAny = false;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t tab = line.find('\t');
            if (tab == std::string::npos) continue;
            std::istringstream ss(line.substr(0, tab));
            Key k = 0;
            if (ss >> k) {
                out->emplace_back(k, line.substr(tab + 1));
                foundAny = true;
            }
        }
        if (foundAny) return true;
        return Fail(out, "bad header");
    }
    if (!in.get(c) || c != '\n') return Fail(out, "bad header");

    // ---- Records ----------------------------------------------------------
    for (unsigned long long i = 0; i < count; ++i) {
        Key key = 0;
        unsigned long long len = 0;

        if (!(in >> key >> len)) return Fail(out, "bad record");  // key + length
        if (!in.get(c) || c != ' ') return Fail(out, "bad record");  // the one space
        if (len > total) return Fail(out, "bad length");

        // Read EXACTLY len bytes, whatever they are (spaces, newlines, ...).
        std::string value(static_cast<size_t>(len), '\0');
        if (len > 0) {
            in.read(&value[0], static_cast<std::streamsize>(len));
            if (static_cast<unsigned long long>(in.gcount()) != len)
                return Fail(out, "truncated value");  // file ended too early
        }

        if (!in.get(c) || c != '\n') return Fail(out, "bad record end");
        out->emplace_back(key, value);
    }
    return true;
}
