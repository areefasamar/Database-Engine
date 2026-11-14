#include "avl_index.h"
#include "cli.h"
#include "database.h"
#include <iostream>
#include <memory>
#include <string>
#include <utility>

int main(int argc, char** argv) {
    std::string file = (argc > 1) ? argv[1] : "mydata.db";

    // The ONE line to change if you want a different index implementation:
    // AVLIndex, SortedArrayIndex or LinkedListIndex.
    std::unique_ptr<Index> index(new AVLIndex());

    Database db(file, std::move(index));
    std::cout << "MiniDB ready. Commands: PUT, GET, DELETE, SCAN, SORT VALUE, "
                 "UNDO, HISTORY, STATS, EXIT\n";
    RunCli(db, std::cin, std::cout);
    return 0;
}
