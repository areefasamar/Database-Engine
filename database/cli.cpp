#include "cli.h"
#include "../data_structures/circular_queue.h"
#include "../parser/parser.h"
#include <istream>
#include <ostream>
#include <iomanip>
#include <string>
#include <vector>

// =========================================================
//  Enable ANSI colours on Windows 10+ (CMD / Windows Terminal)
// =========================================================
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
// windows.h defines DELETE as a macro — that clashes with CommandType::DELETE
#  ifdef DELETE
#    undef DELETE
#  endif
#  ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#    define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#  endif
static void enableAnsiColors() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE || h == NULL) return;
    DWORD mode = 0;
    if (!GetConsoleMode(h, &mode)) return;
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#else
static void enableAnsiColors() {}
#endif

// =========================================================
//  ANSI colour macros  (pure #define, works on every compiler)
// =========================================================
#define C_RESET    "\033[0m"
#define C_BOLD     "\033[1m"
#define C_CYAN     "\033[36m"
#define C_BCYAN    "\033[1;36m"
#define C_GREEN    "\033[32m"
#define C_BGREEN   "\033[1;32m"
#define C_YELLOW   "\033[33m"
#define C_BYELLOW  "\033[1;33m"
#define C_RED      "\033[31m"
#define C_BRED     "\033[1;31m"
#define C_MAGENTA  "\033[35m"
#define C_BMAGENTA "\033[1;35m"
#define C_WHITE    "\033[97m"
#define C_BWHITE   "\033[1;97m"
#define C_GRAY     "\033[90m"

// =========================================================
//  Local helpers
// =========================================================
static const int BOX_W        = 58;
static const std::size_t HIST = 10;

// Full-width horizontal rule
static void hline(std::ostream& out, char ch) {
    out << C_CYAN;
    out << "  +";
    for (int i = 0; i < BOX_W; ++i) out << ch;
    out << "+" C_RESET "\n";
}

// One centred row inside a box:  |   ...text...   |
static void boxRow(std::ostream& out,
                   const std::string& text,
                   const char* col) {
    int inner = BOX_W - 2;
    int pad   = inner - static_cast<int>(text.size());
    int lp    = (pad > 0) ? (pad / 2) : 0;
    int rp    = (pad > 0) ? (pad - lp) : 0;
    out << C_CYAN "  |" C_RESET
        << std::string(lp, ' ')
        << col << text << C_RESET
        << std::string(rp, ' ')
        << C_CYAN "|" C_RESET "\n";
}

static bool isBlank(const std::string& line) {
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return false;
    }
    return true;
}

static bool isHelp(const std::string& raw) {
    // Skip leading spaces, uppercase, strip trailing spaces
    std::string s;
    bool started = false;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        char c = raw[i];
        if (!started && (c == ' ' || c == '\t')) continue;
        started = true;
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);
        s += c;
    }
    while (!s.empty() &&
           (s[s.size()-1] == ' ' || s[s.size()-1] == '\t')) {
        s.erase(s.size()-1);
    }
    return s == "HELP";
}

// =========================================================
//  Banner
// =========================================================
static void printBanner(std::ostream& out) {
    out << "\n";
    hline(out, '=');
    out << C_CYAN "  |" C_RESET
        << std::string(BOX_W, ' ')
        << C_CYAN "|" C_RESET "\n";
    boxRow(out, "M I N I D B   K E Y - V A L U E   E N G I N E", C_BCYAN);
    out << C_CYAN "  |" C_RESET
        << std::string(BOX_W, ' ')
        << C_CYAN "|" C_RESET "\n";
    boxRow(out, "Data Structures & Algorithms  --  CS Project 2024", C_BYELLOW);
    out << C_CYAN "  |" C_RESET
        << std::string(BOX_W, ' ')
        << C_CYAN "|" C_RESET "\n";
    hline(out, '=');
    out << "\n"
        << C_GRAY "  Type " C_BCYAN "HELP" C_GRAY
        " to list commands    |    Type "
        C_BRED "EXIT" C_GRAY " to quit\n" C_RESET;
    hline(out, '-');
    out << "\n";
}

// =========================================================
//  Help table
// =========================================================
static void printHelp(std::ostream& out) {
    out << "\n";
    hline(out, '=');
    boxRow(out, "COMMAND  REFERENCE  CARD", C_BCYAN);
    hline(out, '=');

    struct CmdRow { const char* cmd; const char* desc; };
    static const CmdRow rows[] = {
        { "PUT <key> <value>",  "Insert or update a record"           },
        { "GET <key>",          "Retrieve a record by key"            },
        { "DELETE <key>",       "Remove a record by key"              },
        { "SCAN <lo> <hi>",     "List all keys in range [lo, hi]"    },
        { "SORT VALUE",         "List all records sorted by value"    },
        { "UNDO",               "Reverse the last PUT or DELETE"      },
        { "HISTORY",            "Show last 10 commands typed"         },
        { "STATS",              "Record count, undo depth, file name" },
        { "SAVE",               "Flush all records to disk right now" },
        { "HELP",               "Show this reference card"            },
        { "EXIT",               "Quit MiniDB (auto-saves)"           },
    };

    out << C_CYAN "  |  " C_BYELLOW
        << std::left << std::setw(20) << "Command"
        << C_GRAY "  Description\n" C_RESET;
    hline(out, '-');

    for (std::size_t i = 0; i < sizeof(rows)/sizeof(rows[0]); ++i) {
        out << C_CYAN "  |  " C_BGREEN
            << std::left << std::setw(20) << rows[i].cmd
            << C_WHITE "  " << rows[i].desc
            << C_RESET "\n";
    }
    hline(out, '-');
    out << C_GRAY
        "  Keys are integers. "
        "Values with spaces must be quoted: "
        C_BWHITE "PUT 1 \"hello world\"\n"
        C_RESET "\n";
}

// =========================================================
//  Record table  (SCAN / SORT output)
// =========================================================
static void printRecords(std::ostream& out,
                         const std::vector<KeyValue>& recs) {
    if (recs.empty()) {
        out << "  " C_BYELLOW "[>>]" C_RESET " No records found.\n";
        return;
    }
    out << "\n";
    hline(out, '-');
    out << C_CYAN "  |  " C_BYELLOW
        << std::left << std::setw(14) << "KEY"
        << "VALUE\n" C_RESET;
    hline(out, '-');
    for (std::size_t i = 0; i < recs.size(); ++i) {
        out << C_CYAN "  |  " C_BGREEN
            << std::right << std::setw(8) << recs[i].first
            << C_GRAY "  =>  " C_WHITE
            << recs[i].second
            << C_RESET "\n";
    }
    hline(out, '-');
    out << C_GRAY "  " << recs.size()
        << " record(s) returned.\n" C_RESET "\n";
}

// =========================================================
//  History
// =========================================================
static void printHistory(std::ostream& out,
                         const CircularQueue<std::string>& history) {
    if (history.Size() == 0) {
        out << "  " C_BYELLOW "[>>]" C_RESET " No history yet.\n";
        return;
    }
    out << "\n" C_BYELLOW "  Recent commands:\n" C_RESET;
    hline(out, '-');
    for (std::size_t i = 0; i < history.Size(); ++i) {
        std::string ln;
        history.At(i, &ln);
        out << C_GRAY "  " << std::setw(3) << (i + 1) << ".  "
            << C_WHITE << ln << C_RESET "\n";
    }
    hline(out, '-');
    out << "\n";
}

// =========================================================
//  Stats panel
// =========================================================
static void printStats(std::ostream& out, const Database& db) {
    out << "\n";
    hline(out, '=');
    boxRow(out, "DATABASE  STATISTICS", C_BCYAN);
    hline(out, '-');
    out << C_CYAN "  |  " C_BYELLOW "Records stored   : "
        C_BGREEN << db.Size()      << C_RESET "\n";
    out << C_CYAN "  |  " C_BYELLOW "Undo stack depth : "
        C_BGREEN << db.UndoDepth() << C_RESET "\n";
    out << C_CYAN "  |  " C_BYELLOW "LRU Cache usage  : "
        C_BGREEN << db.CacheSize() << "/" << db.CacheCapacity() << C_RESET "\n";
    out << C_CYAN "  |  " C_BYELLOW "Cache Hits       : "
        C_BGREEN << db.CacheHits() << C_RESET "\n";
    out << C_CYAN "  |  " C_BYELLOW "Cache Misses     : "
        C_BGREEN << db.CacheMisses() << C_RESET "\n";
    out << C_CYAN "  |  " C_BYELLOW "Data file        : "
        C_WHITE  << db.FileName()  << C_RESET "\n";
    hline(out, '=');
    out << "\n";
}

// =========================================================
//  Main REPL
// =========================================================
void RunCli(Database& db, std::istream& in, std::ostream& out,
            const std::string& prompt) {

    enableAnsiColors();

    if (!prompt.empty()) {
        printBanner(out);
    }

    CircularQueue<std::string> history(HIST);
    std::string line;

    while (true) {

        if (!prompt.empty()) {
            out << C_BMAGENTA "  minidb" C_GRAY " > " C_RESET;
            out.flush();
        }

        if (!std::getline(in, line)) break;
        if (isBlank(line))           continue;

        history.Push(line);

        if (isHelp(line)) { printHelp(out); continue; }

        Command cmd = ParseLine(line);

        switch (cmd.type) {

            case CommandType::PUT:
                if (db.Put(cmd.key, cmd.value)) {
                    out << "  " C_BGREEN "[OK] " C_RESET
                        "Inserted  " C_CYAN "key=" << cmd.key
                        << C_GRAY "  =>  " C_WHITE
                        "\"" << cmd.value << "\"" C_RESET "\n";
                } else {
                    out << "  " C_BGREEN "[OK] " C_RESET
                        "Updated   " C_CYAN "key=" << cmd.key
                        << C_GRAY "  =>  " C_WHITE
                        "\"" << cmd.value << "\"" C_RESET "\n";
                }
                break;

            case CommandType::GET: {
                std::string val;
                if (db.Get(cmd.key, &val)) {
                    out << "  " C_BGREEN "[OK] " C_RESET
                        C_CYAN "key=" << cmd.key
                        << C_GRAY "  =>  " C_WHITE
                        "\"" << val << "\"" C_RESET "\n";
                } else {
                    out << "  " C_BRED "[!!] " C_RESET
                        C_YELLOW "Key " << cmd.key
                        << " not found." C_RESET "\n";
                }
                break;
            }

            case CommandType::DELETE:
                if (db.Delete(cmd.key)) {
                    out << "  " C_BGREEN "[OK] " C_RESET
                        "Deleted  " C_CYAN "key=" << cmd.key
                        << C_RESET "\n";
                } else {
                    out << "  " C_BRED "[!!] " C_RESET
                        C_YELLOW "Key " << cmd.key
                        << " not found." C_RESET "\n";
                }
                break;

            case CommandType::SCAN:
                out << "  " C_BYELLOW "[>>] " C_RESET
                    "Scanning keys "
                    C_CYAN << cmd.key << C_RESET " to "
                    C_CYAN << cmd.hi  << C_RESET " ...\n";
                printRecords(out, db.Scan(cmd.key, cmd.hi));
                break;

            case CommandType::SORT:
                out << "  " C_BYELLOW "[>>] " C_RESET
                    "Sorting all records by value ...\n";
                printRecords(out, db.SortedByValue());
                break;

            case CommandType::UNDO:
                if (db.Undo()) {
                    out << "  " C_BGREEN "[OK] " C_RESET
                        "Last change undone.\n";
                } else {
                    out << "  " C_BYELLOW "[>>] " C_RESET
                        "Nothing to undo.\n";
                }
                break;

            case CommandType::HISTORY:
                printHistory(out, history);
                break;

            case CommandType::STATS:
                printStats(out, db);
                break;

            case CommandType::SAVE:
                if (db.Save()) {
                    out << "  " C_BGREEN "[OK] " C_RESET
                        "Saved to " C_CYAN << db.FileName()
                        << C_RESET "\n";
                } else {
                    out << "  " C_BRED "[!!] " C_RESET
                        C_YELLOW "Could not write to file: "
                        << db.FileName() << C_RESET "\n";
                }
                break;

            case CommandType::EXIT:
                if (!prompt.empty()) {
                    out << "\n";
                    hline(out, '=');
                    boxRow(out,
                           "Thank you for using MiniDB!  Goodbye :)",
                           C_BGREEN);
                    hline(out, '=');
                    out << "\n";
                }
                return;

            case CommandType::INVALID:
                out << "  " C_BRED "[!!] " C_RESET
                    C_YELLOW << cmd.error
                    << C_GRAY "  (type HELP)\n" C_RESET;
                break;
        }
    }
}
