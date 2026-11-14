#ifndef MINIDB_PARSER_H
#define MINIDB_PARSER_H

#include "common.h"
#include <string>
#include <vector>

// Turns one line of text into a Command. Plain string handling, no libraries.
//
// Grammar (command names are case-insensitive):
//   PUT <int-key> <value>     value is one word or a "quoted string"
//   GET <int-key>
//   DELETE <int-key>
//   SCAN <lo> <hi>
//   SORT VALUE
//   UNDO
//   HISTORY
//   STATS
//   EXIT
//
// Quotes are only allowed around the PUT value. There are no escape
// sequences, so a value cannot contain a double quote character.

enum class CommandType {
    PUT, GET, DELETE, SCAN, SORT, UNDO, HISTORY, STATS, EXIT, INVALID
};

struct Command {
    CommandType type = CommandType::INVALID;
    Key key = 0;          // PUT, GET, DELETE: the key. SCAN: the low end.
    Key hi = 0;           // SCAN: the high end.
    std::string value;    // PUT: the value.
    std::string error;    // INVALID only: a helpful message. Empty otherwise.
};

// One piece of the input line. "quoted" is true if it was written in quotes.
struct ParserToken {
    std::string text;
    bool quoted;
};

// Splits a line into tokens. Spaces and tabs separate tokens. A token that
// starts with a double quote runs until the closing quote, so it may contain
// spaces (and may be empty). Returns false and fills *error on bad input
// (unterminated quote, or text stuck to a closing quote).
bool Tokenize(const std::string& line, std::vector<ParserToken>* tokens,
              std::string* error);

// Never throws and never crashes. Bad input gives type == INVALID with a
// message in error. A blank line also gives INVALID ("Empty command."), so
// callers can skip blank lines before parsing if they prefer.
Command ParseLine(const std::string& line);

#endif
