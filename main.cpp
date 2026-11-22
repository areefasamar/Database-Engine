#include "index/avl_index.h"
#include "database/cli.h"
#include "database/database.h"
#include <iostream>
#include <memory>
#include <string>
#include <utility>

int main(int argc, char** argv) {
    try {
        std::string file = (argc > 1) ? argv[1] : "mydata.db";

        // The ONE line to change if you want a different index implementation:
        // AVLIndex, SortedArrayIndex or LinkedListIndex.
        std::unique_ptr<Index> index(new AVLIndex());

        Database db(file, std::move(index));
        // The banner + prompt are printed by RunCli itself.
        RunCli(db, std::cin, std::cout);
    } catch (const std::exception& ex) {
        std::cerr << "Fatal Error: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal Error: Unknown exception occurred." << std::endl;
        return 2;
    }
    return 0;
}
