
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>

#include "DiagnosticCsv.h"

int main() {
    DiagnosticBuffer log;

    // Simulated sensor diagnostic events
    log.add(1001, FaultSeverity::WARNING, 1001);
    log.add(1002, FaultSeverity::INFO, 1600);
    log.add(1001, FaultSeverity::WARNING, 2601);

    const std::string filename =
        "TestBench/src/diagnostic_log.csv";

    // Export events to CSV.
    bool success = exportDiagnosticCsv(log, filename);
    assert(success);

    // Read the generated file back.
    std::ifstream file(filename);
    assert(file.is_open());

    std::stringstream contents;
    contents << file.rdbuf();

    const std::string expected =
        "timestamp_ms,sensor_id,event_code,severity\n"
        "1001,0,1001,WARNING\n"
        "1600,0,1002,INFO\n"
        "2601,0,1001,WARNING\n";

    // Confirm the exported data is correct.
    assert(contents.str() == expected);

    // Exporting must not clear the RAM buffer.
    assert(log.size() == 3);

    std::cout << "CSV export test PASSED\n";
    std::cout << "Recorded events: " << log.size() << "\n";
    std::cout << "File created: " << filename << "\n";
    std::cout << "\nCSV contents:\n";
    std::cout << contents.str();

    return 0;
}
