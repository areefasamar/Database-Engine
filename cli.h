#ifndef MINIDB_CLI_H
#define MINIDB_CLI_H

#include "database.h"
#include <iosfwd>
#include <string>

// Reads commands from `in` until EXIT or end of input and writes results to
// `out`. Invalid input prints an error and the loop continues.
// HISTORY shows the last 10 non-blank lines.
// Pass an empty prompt in tests so the output only holds results.
void RunCli(Database& db, std::istream& in, std::ostream& out,
            const std::string& prompt = "> ");

#endif
