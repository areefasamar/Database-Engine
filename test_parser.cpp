#include "parser.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

using namespace std;

// Checks stay active even if NDEBUG is enabled.
void check(bool condition, const char* message) {
    if (!condition) {
        cerr << "FAILED: " << message << endl;
        exit(1);
    }
}

bool isInvalid(const string& line) {
    Command c = ParseLine(line);
    return c.type == CommandType::INVALID && !c.error.empty();
}

void testValidCommands() {
    Command c = ParseLine("PUT 101 Areefa");
    check(c.type == CommandType::PUT && c.key == 101 && c.value == "Areefa"
          && c.error.empty(), "PUT with a word");

    c = ParseLine("GET 101");
    check(c.type == CommandType::GET && c.key == 101 && c.error.empty(), "GET");

    c = ParseLine("DELETE 7");
    check(c.type == CommandType::DELETE && c.key == 7, "DELETE");

    c = ParseLine("SCAN 100 200");
    check(c.type == CommandType::SCAN && c.key == 100 && c.hi == 200, "SCAN");

    c = ParseLine("SORT VALUE");
    check(c.type == CommandType::SORT && c.error.empty(), "SORT VALUE");

    check(ParseLine("UNDO").type == CommandType::UNDO, "UNDO");
    check(ParseLine("HISTORY").type == CommandType::HISTORY, "HISTORY");
    check(ParseLine("STATS").type == CommandType::STATS, "STATS");
    check(ParseLine("EXIT").type == CommandType::EXIT, "EXIT");
}

void testCaseInsensitive() {
    check(ParseLine("put 1 a").type == CommandType::PUT, "lowercase put");
    check(ParseLine("Get 1").type == CommandType::GET, "mixed case get");
    check(ParseLine("dElEtE 1").type == CommandType::DELETE, "odd case delete");
    check(ParseLine("scan 1 2").type == CommandType::SCAN, "lowercase scan");
    check(ParseLine("sort value").type == CommandType::SORT, "lowercase sort value");
    check(ParseLine("undo").type == CommandType::UNDO, "lowercase undo");
    check(ParseLine("exit").type == CommandType::EXIT, "lowercase exit");
    // The value keeps its case.
    check(ParseLine("put 1 MiXeD").value == "MiXeD", "value case is preserved");
}

void testQuotedValues() {
    Command c = ParseLine("PUT 5 \"Muhammad Abdullah\"");
    check(c.type == CommandType::PUT && c.value == "Muhammad Abdullah",
          "quoted value with a space");

    c = ParseLine("PUT 5 \"\"");
    check(c.type == CommandType::PUT && c.value.empty() && c.error.empty(),
          "empty quoted value");

    c = ParseLine("PUT 5 \"  padded  \"");
    check(c.value == "  padded  ", "spaces inside quotes are kept");

    c = ParseLine("PUT 5 \"GET 1\"");
    check(c.type == CommandType::PUT && c.value == "GET 1",
          "value that looks like a command");

    c = ParseLine("PUT 5 \"tab\there\"");
    check(c.value == "tab\there", "tab inside quotes is kept");

    c = ParseLine("PUT 5 caf\xC3\xA9");
    check(c.type == CommandType::PUT && c.value == "caf\xC3\xA9", "UTF-8 bytes");

    c = ParseLine("PUT 5 a\"b");
    check(c.type == CommandType::PUT && c.value == "a\"b",
          "quote in the middle of a bare word is plain text");
}

void testNumbers() {
    check(ParseLine("GET -42").key == -42, "negative key");
    check(ParseLine("GET +42").key == 42, "plus sign key");
    check(ParseLine("GET 0").key == 0, "zero key");

    Key low = numeric_limits<Key>::min();
    Key high = numeric_limits<Key>::max();
    check(ParseLine("GET -9223372036854775808").key == low, "int64 minimum");
    check(ParseLine("GET 9223372036854775807").key == high, "int64 maximum");
    check(ParseLine("SCAN -5 5").key == -5, "SCAN negative low end");

    // A reversed range is a valid command. The index returns nothing for it.
    Command c = ParseLine("SCAN 200 100");
    check(c.type == CommandType::SCAN && c.key == 200 && c.hi == 100,
          "reversed SCAN range parses");
}

void testBadNumbers() {
    check(isInvalid("GET abc"), "non-numeric key");
    check(isInvalid("GET 12abc"), "key with trailing letters");
    check(isInvalid("GET 1.5"), "decimal key");
    check(isInvalid("GET 1e3"), "scientific notation key");
    check(isInvalid("GET -"), "lone minus");
    check(isInvalid("GET 99999999999999999999"), "huge positive number");
    check(isInvalid("GET -99999999999999999999"), "huge negative number");
    check(isInvalid("GET 9223372036854775808"), "one past int64 max");
    check(isInvalid("PUT x value"), "PUT with non-numeric key");
    check(isInvalid("DELETE one"), "DELETE with non-numeric key");
    check(isInvalid("SCAN 1 two"), "SCAN with non-numeric high");
    check(isInvalid("SCAN one 2"), "SCAN with non-numeric low");
}

void testArgumentCounts() {
    check(isInvalid("PUT"), "PUT with nothing");
    check(isInvalid("PUT 1"), "PUT without a value");
    check(isInvalid("PUT 1 two words"), "PUT with unquoted spaces");
    check(isInvalid("GET"), "GET without a key");
    check(isInvalid("GET 1 2"), "GET with two keys");
    check(isInvalid("DELETE"), "DELETE without a key");
    check(isInvalid("DELETE 1 2"), "DELETE with two keys");
    check(isInvalid("SCAN"), "SCAN with nothing");
    check(isInvalid("SCAN 1"), "SCAN with one number");
    check(isInvalid("SCAN 1 2 3"), "SCAN with three numbers");
    check(isInvalid("SORT"), "SORT alone");
    check(isInvalid("SORT KEY"), "SORT with wrong field");
    check(isInvalid("SORT VALUE EXTRA"), "SORT VALUE with extra");
    check(isInvalid("UNDO now"), "UNDO with an argument");
    check(isInvalid("HISTORY 5"), "HISTORY with an argument");
    check(isInvalid("STATS all"), "STATS with an argument");
    check(isInvalid("EXIT 0"), "EXIT with an argument");
}

void testQuoteMisuse() {
    check(isInvalid("PUT 1 \"unterminated"), "unterminated quote");
    check(isInvalid("PUT 1 \""), "lone opening quote");
    check(isInvalid("PUT 1 \"abc\"def"), "text stuck to closing quote");
    check(isInvalid("PUT \"1\" value"), "quoted key");
    check(isInvalid("GET \"1\""), "quoted GET key");
    check(isInvalid("\"PUT\" 1 a"), "quoted command name");
    check(isInvalid("SORT \"VALUE\""), "quoted SORT field");

    Command c = ParseLine("PUT 1 \"abc");
    check(c.error.find("quote") != string::npos, "quote error mentions quote");
}

void testBlankAndUnknown() {
    check(isInvalid(""), "empty line");
    check(isInvalid("   "), "spaces only");
    check(isInvalid("\t\t"), "tabs only");
    check(isInvalid("\r\n"), "line break only");
    check(isInvalid("FLY 1"), "unknown command");
    check(isInvalid("PUTT 1 a"), "near miss command");
    check(isInvalid("123"), "number alone");
    check(isInvalid("FLY \"quoted\""), "unknown command with quotes");

    Command c = ParseLine("FLY 1");
    check(c.error.find("FLY") != string::npos, "unknown command is named");
}

void testWhitespaceHandling() {
    Command c = ParseLine("   PUT    7    seven   ");
    check(c.type == CommandType::PUT && c.key == 7 && c.value == "seven",
          "extra spaces");
    c = ParseLine("\tGET\t7\t");
    check(c.type == CommandType::GET && c.key == 7, "tabs between tokens");
    c = ParseLine("GET 7\r\n");
    check(c.type == CommandType::GET && c.key == 7, "Windows line ending");
    c = ParseLine("PUT 7 \"a  b\"   ");
    check(c.value == "a  b", "trailing spaces after a quoted value");
}

void testTokenizer() {
    vector<ParserToken> tokens;
    string error;
    check(Tokenize("", &tokens, &error) && tokens.empty(), "tokenize empty");
    check(Tokenize("a b  c", &tokens, &error) && tokens.size() == 3,
          "tokenize three words");
    check(Tokenize("a \"b c\" d", &tokens, &error) && tokens.size() == 3
          && tokens[1].text == "b c" && tokens[1].quoted && !tokens[0].quoted,
          "tokenize quoted middle token");
    check(Tokenize("\"\"", &tokens, &error) && tokens.size() == 1
          && tokens[0].text.empty() && tokens[0].quoted, "tokenize empty quotes");
    check(!Tokenize("a \"b", &tokens, &error) && !error.empty(),
          "tokenize unterminated quote");
    check(Tokenize("ok", &tokens, &error) && tokens.size() == 1,
          "tokens are reset between calls");
}

// Random garbage must never crash, and the result must always be consistent:
// INVALID has a message, everything else has none.
void testFuzz() {
    mt19937 rng(424242);
    const string alphabet = "PUTGEDLSCANOHISRYXabc019 -+\"\t.\r";
    for (int round = 0; round < 30000; ++round) {
        size_t length = rng() % 24;
        string line;
        for (size_t i = 0; i < length; ++i) {
            line += alphabet[rng() % alphabet.size()];
        }
        Command c = ParseLine(line);
        if (c.type == CommandType::INVALID) {
            check(!c.error.empty(), "INVALID always has a message");
        } else {
            check(c.error.empty(), "valid commands have no message");
        }
    }

    // Random binary junk and very long lines.
    for (int round = 0; round < 2000; ++round) {
        string line;
        size_t length = rng() % 200;
        for (size_t i = 0; i < length; ++i) {
            line += static_cast<char>(rng() % 256);
        }
        Command c = ParseLine(line);
        check(c.type == CommandType::INVALID ? !c.error.empty() : c.error.empty(),
              "binary junk stays consistent");
    }
    string longValue(100000, 'x');
    Command c = ParseLine("PUT 1 " + longValue);
    check(c.type == CommandType::PUT && c.value.size() == 100000, "very long value");
}

int main() {
    testValidCommands();
    testCaseInsensitive();
    testQuotedValues();
    testNumbers();
    testBadNumbers();
    testArgumentCounts();
    testQuoteMisuse();
    testBlankAndUnknown();
    testWhitespaceHandling();
    testTokenizer();
    testFuzz();
    cout << "All parser tests passed." << endl;
    return 0;
}
