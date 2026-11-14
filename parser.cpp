#include "parser.h"
#include <cctype>
#include <exception>
#include <stdexcept>

namespace {

bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

std::string toUpper(const std::string& text) {
    std::string result = text;
    for (std::size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(result[i])));
    }
    return result;
}

Command makeError(const std::string& message) {
    Command command;
    command.type = CommandType::INVALID;
    command.error = message;
    return command;
}

// Converts the whole text to a Key. Rejects "12abc", "", "1.5" and values
// that do not fit in 64 bits.
bool parseKey(const std::string& text, const char* what, Key* out,
              std::string* error) {
    try {
        std::size_t used = 0;
        long long number = std::stoll(text, &used);
        if (used != text.size()) {
            *error = std::string(what) + " must be a whole number, got '"
                     + text + "'.";
            return false;
        }
        *out = static_cast<Key>(number);
        return true;
    } catch (const std::invalid_argument&) {
        *error = std::string(what) + " must be a whole number, got '"
                 + text + "'.";
    } catch (const std::out_of_range&) {
        *error = std::string(what) + " is too large or too small: '"
                 + text + "'.";
    }
    return false;
}

Command parseTokens(const std::vector<ParserToken>& tokens) {
    if (tokens.empty()) {
        return makeError("Empty command.");
    }

    std::string name = toUpper(tokens[0].text);
    bool known = name == "PUT" || name == "GET" || name == "DELETE"
                 || name == "SCAN" || name == "SORT" || name == "UNDO"
                 || name == "HISTORY" || name == "STATS" || name == "EXIT";

    // Quotes are only meaningful around the PUT value (token 2).
    if (known) {
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            bool isPutValue = (name == "PUT" && i == 2);
            if (tokens[i].quoted && !isPutValue) {
                return makeError(
                    "Quotes are only allowed around the value in PUT.");
            }
        }
    }

    Command command;
    std::string error;

    if (name == "PUT") {
        if (tokens.size() < 3) {
            return makeError("Usage: PUT <key> <value>");
        }
        if (tokens.size() > 3) {
            return makeError("Too many arguments. Put values with spaces in "
                             "quotes: PUT 1 \"two words\"");
        }
        if (!parseKey(tokens[1].text, "Key", &command.key, &error)) {
            return makeError(error);
        }
        command.type = CommandType::PUT;
        command.value = tokens[2].text;
        return command;
    }

    if (name == "GET" || name == "DELETE") {
        if (tokens.size() != 2) {
            return makeError("Usage: " + name + " <key>");
        }
        if (!parseKey(tokens[1].text, "Key", &command.key, &error)) {
            return makeError(error);
        }
        command.type = (name == "GET") ? CommandType::GET : CommandType::DELETE;
        return command;
    }

    if (name == "SCAN") {
        if (tokens.size() != 3) {
            return makeError("Usage: SCAN <lo> <hi>");
        }
        if (!parseKey(tokens[1].text, "Low key", &command.key, &error)
            || !parseKey(tokens[2].text, "High key", &command.hi, &error)) {
            return makeError(error);
        }
        command.type = CommandType::SCAN;
        return command;
    }

    if (name == "SORT") {
        if (tokens.size() != 2 || toUpper(tokens[1].text) != "VALUE") {
            return makeError("Usage: SORT VALUE");
        }
        command.type = CommandType::SORT;
        return command;
    }

    if (name == "UNDO" || name == "HISTORY" || name == "STATS"
        || name == "EXIT") {
        if (tokens.size() != 1) {
            return makeError(name + " takes no arguments.");
        }
        if (name == "UNDO") {
            command.type = CommandType::UNDO;
        } else if (name == "HISTORY") {
            command.type = CommandType::HISTORY;
        } else if (name == "STATS") {
            command.type = CommandType::STATS;
        } else {
            command.type = CommandType::EXIT;
        }
        return command;
    }

    return makeError("Unknown command '" + tokens[0].text + "'. Commands: PUT, "
                     "GET, DELETE, SCAN, SORT VALUE, UNDO, HISTORY, STATS, EXIT.");
}

}  // namespace

bool Tokenize(const std::string& line, std::vector<ParserToken>* tokens,
              std::string* error) {
    tokens->clear();
    std::size_t i = 0;
    const std::size_t n = line.size();

    while (i < n) {
        if (isSpace(line[i])) {
            ++i;
            continue;
        }

        ParserToken token;
        token.quoted = false;

        if (line[i] == '"') {
            token.quoted = true;
            ++i;  // skip the opening quote
            while (i < n && line[i] != '"') {
                token.text += line[i];
                ++i;
            }
            if (i >= n) {
                *error = "Unterminated quote: add a closing \" at the end.";
                return false;
            }
            ++i;  // skip the closing quote
            if (i < n && !isSpace(line[i])) {
                *error = "Unexpected text right after a closing quote.";
                return false;
            }
        } else {
            while (i < n && !isSpace(line[i])) {
                token.text += line[i];
                ++i;
            }
        }
        tokens->push_back(token);
    }
    return true;
}

Command ParseLine(const std::string& line) {
    try {
        std::vector<ParserToken> tokens;
        std::string error;
        if (!Tokenize(line, &tokens, &error)) {
            return makeError(error);
        }
        return parseTokens(tokens);
    } catch (const std::exception&) {
        return makeError("Could not read that command.");
    }
}
