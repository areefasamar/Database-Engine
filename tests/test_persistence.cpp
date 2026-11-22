#include "../database/persistence.h"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cstdio>

int main() {
    const std::string testFile = "test_persistence.tmp.db";

    std::vector<KeyValue> written = {
        {101, "Areefa"},
        {102, "Arbish"},
        {103, "Areeba"},
        {104, "Value with spaces and symbols! @#$%"}
    };

    assert(Persistence::Save(testFile, written) == true);

    std::vector<KeyValue> loaded;
    assert(Persistence::Load(testFile, &loaded) == true);

    assert(loaded.size() == written.size());
    for (size_t i = 0; i < written.size(); ++i) {
        assert(loaded[i].first == written[i].first);
        assert(loaded[i].second == written[i].second);
    }

    std::remove(testFile.c_str());
    std::cout << "All Persistence tests passed!\n";
    return 0;
}
