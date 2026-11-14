#include "cli.h"
#include "circular_queue.h"
#include "parser.h"
#include <istream>
#include <ostream>

namespace {

const std::size_t kHistorySize = 10;

bool isBlank(const std::string& line) {
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            return false;
        }
    }
    return true;
}

void printRecords(std::ostream& out, const std::vector<KeyValue>& records) {
    if (records.empty()) {
        out << "No records found.\n";
        return;
    }
    for (std::size_t i = 0; i < records.size(); ++i) {
        out << records[i].first << " -> " << records[i].second << "\n";
    }
}

void printHistory(std::ostream& out, const CircularQueue<std::string>& history) {
    for (std::size_t i = 0; i < history.Size(); ++i) {
        std::string line;
        history.At(i, &line);
        out << (i + 1) << ". " << line << "\n";
    }
}

}  // namespace

void RunCli(Database& db, std::istream& in, std::ostream& out,
            const std::string& prompt) {
    CircularQueue<std::string> history(kHistorySize);
    std::string line;

    while (true) {
        out << prompt;
        out.flush();
        if (!std::getline(in, line)) {
            break;  // end of input behaves like EXIT
        }
        if (isBlank(line)) {
            continue;
        }
        history.Push(line);

        Command command = ParseLine(line);
        switch (command.type) {
            case CommandType::PUT:
                if (db.Put(command.key, command.value)) {
                    out << "SUCCESS: Record " << command.key << " inserted.\n";
                } else {
                    out << "SUCCESS: Record " << command.key << " updated.\n";
                }
                break;
            case CommandType::GET: {
                std::string value;
                if (db.Get(command.key, &value)) {
                    out << command.key << " -> " << value << "\n";
                } else {
                    out << "NOT FOUND\n";
                }
                break;
            }
            case CommandType::DELETE:
                if (db.Delete(command.key)) {
                    out << "SUCCESS: Record " << command.key << " deleted.\n";
                } else {
                    out << "NOT FOUND\n";
                }
                break;
            case CommandType::SCAN:
                printRecords(out, db.Scan(command.key, command.hi));
                break;
            case CommandType::SORT:
                printRecords(out, db.SortedByValue());
                break;
            case CommandType::UNDO:
                if (db.Undo()) {
                    out << "SUCCESS: Undid last change.\n";
                } else {
                    out << "Nothing to undo.\n";
                }
                break;
            case CommandType::HISTORY:
                printHistory(out, history);
                break;
            case CommandType::STATS:
                out << "records: " << db.Size()
                    << " | undo steps: " << db.UndoDepth()
                    << " | cache: not added yet\n";
                break;
            case CommandType::EXIT:
                out << "Goodbye.\n";
                return;
            case CommandType::INVALID:
                out << "ERROR: " << command.error << "\n";
                break;
        }
    }
}
