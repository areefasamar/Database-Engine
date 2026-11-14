#include "avl_index.h"
#include "cli.h"
#include "database.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace std;

// Checks stay active even if NDEBUG is enabled.
void check(bool condition, const char* message) {
    if (!condition) {
        cerr << "FAILED: " << message << endl;
        exit(1);
    }
}

// Runs a scripted session (no prompt) and returns everything printed.
string runSession(const string& input) {
    Database db("unused.db", unique_ptr<Index>(new AVLIndex()));
    istringstream in(input);
    ostringstream out;
    RunCli(db, in, out, "");
    return out.str();
}

void expectSession(const string& input, const string& expected,
                   const char* message) {
    string actual = runSession(input);
    if (actual != expected) {
        cerr << "FAILED: " << message << "\n--- expected ---\n" << expected
             << "--- actual ---\n" << actual << "----------------" << endl;
        exit(1);
    }
}

void testWalkthrough() {
    expectSession(
        "PUT 101 \"Areefa\"\n"
        "PUT 102 Ali\n"
        "GET 101\n"
        "SCAN 100 200\n"
        "DELETE 101\n"
        "GET 101\n"
        "UNDO\n"
        "GET 101\n"
        "SORT VALUE\n"
        "PUT 102 \"Ali Khan\"\n"
        "STATS\n"
        "EXIT\n",
        "SUCCESS: Record 101 inserted.\n"
        "SUCCESS: Record 102 inserted.\n"
        "101 -> Areefa\n"
        "101 -> Areefa\n"
        "102 -> Ali\n"
        "SUCCESS: Record 101 deleted.\n"
        "NOT FOUND\n"
        "SUCCESS: Undid last change.\n"
        "101 -> Areefa\n"
        "102 -> Ali\n"
        "101 -> Areefa\n"
        "SUCCESS: Record 102 updated.\n"
        "records: 2 | undo steps: 3 | cache: not added yet\n"
        "Goodbye.\n",
        "README walkthrough session");
}

void testErrorsKeepTheLoopRunning() {
    expectSession(
        "FLY 1\n"
        "GET abc\n"
        "PUT 1\n"
        "PUT 1 \"open\n"
        "GET 1\n"
        "DELETE 1\n"
        "UNDO\n"
        "EXIT\n",
        "ERROR: Unknown command 'FLY'. Commands: PUT, GET, DELETE, SCAN, "
        "SORT VALUE, UNDO, HISTORY, STATS, EXIT.\n"
        "ERROR: Key must be a whole number, got 'abc'.\n"
        "ERROR: Usage: PUT <key> <value>\n"
        "ERROR: Unterminated quote: add a closing \" at the end.\n"
        "NOT FOUND\n"
        "NOT FOUND\n"
        "Nothing to undo.\n"
        "Goodbye.\n",
        "errors do not stop the loop");
}

void testBlankLinesAndWindowsLineEndings() {
    expectSession("\n   \n\t\nPUT 1 a\r\nGET 1\r\n\r\nEXIT\r\n",
                  "SUCCESS: Record 1 inserted.\n"
                  "1 -> a\n"
                  "Goodbye.\n",
                  "blank lines are skipped and CRLF works");
}

void testEmptyResults() {
    expectSession("SCAN 1 10\nSORT VALUE\nPUT 5 x\nSCAN 6 9\nSCAN 9 6\nEXIT\n",
                  "No records found.\n"
                  "No records found.\n"
                  "SUCCESS: Record 5 inserted.\n"
                  "No records found.\n"
                  "No records found.\n"
                  "Goodbye.\n",
                  "empty results are announced");
}

void testStopsAtExitAndAtEndOfInput() {
    expectSession("PUT 1 a\nEXIT\nPUT 2 b\nGET 1\n",
                  "SUCCESS: Record 1 inserted.\nGoodbye.\n",
                  "nothing runs after EXIT");
    expectSession("PUT 1 a\nGET 1",
                  "SUCCESS: Record 1 inserted.\n1 -> a\n",
                  "end of input ends the loop (no trailing newline)");
    expectSession("", "", "empty input");
}

void testHistory() {
    expectSession("PUT 1 a\nGET 1\nHISTORY\nEXIT\n",
                  "SUCCESS: Record 1 inserted.\n"
                  "1 -> a\n"
                  "1. PUT 1 a\n"
                  "2. GET 1\n"
                  "3. HISTORY\n"
                  "Goodbye.\n",
                  "history lists commands oldest to newest");

    // 14 commands, then HISTORY: only the last 10 lines are kept.
    string script;
    for (int i = 1; i <= 14; ++i) {
        script += "GET " + to_string(i) + "\n";
    }
    script += "HISTORY\nEXIT\n";
    string out = runSession(script);
    check(out.find("1. GET 6\n") != string::npos, "history starts at command 6");
    check(out.find("9. GET 14\n") != string::npos, "history has the newest GET");
    check(out.find("10. HISTORY\n") != string::npos, "history includes itself");
    check(out.find("GET 5\n1.") == string::npos
          && out.find("11. ") == string::npos, "history keeps exactly 10");

    // Invalid lines are history too.
    expectSession("FLY\nHISTORY\nEXIT\n",
                  "ERROR: Unknown command 'FLY'. Commands: PUT, GET, DELETE, "
                  "SCAN, SORT VALUE, UNDO, HISTORY, STATS, EXIT.\n"
                  "1. FLY\n2. HISTORY\nGoodbye.\n",
                  "invalid lines are recorded in history");
}

void testPrompt() {
    Database db("unused.db", unique_ptr<Index>(new AVLIndex()));
    istringstream in("GET 1\nEXIT\n");
    ostringstream out;
    RunCli(db, in, out);  // default prompt
    check(out.str() == "> NOT FOUND\n> Goodbye.\n", "default prompt is '> '");
}

int main() {
    testWalkthrough();
    testErrorsKeepTheLoopRunning();
    testBlankLinesAndWindowsLineEndings();
    testEmptyResults();
    testStopsAtExitAndAtEndOfInput();
    testHistory();
    testPrompt();
    cout << "All CLI tests passed." << endl;
    return 0;
}
