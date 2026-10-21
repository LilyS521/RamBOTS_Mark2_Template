
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "../../Teensy/sensors/SensorMonitor.h"
#include "DiagnosticCsv.h"

int main() {
    std::cout << "RamBOTs Mark II - Integrated Diagnostic Test\n\n";

    // Initialize the diagnostic system.
    DiagnosticBuffer log;
    SensorMonitor sensor(log, 1000);

    // 1. Sensor begins reporting normally.
    sensor.receiveUpdate(0);
    sensor.check(500);

    assert(!sensor.isTimedOut());
    assert(log.empty());

    std::cout << "[500 ms] Sensor operating normally\n";

    // 2. Sensor stops reporting.
    sensor.check(1001);

    assert(sensor.isTimedOut());
    assert(log.size() == 1);

    std::cout << "[1001 ms] Sensor timeout detected\n";

    // 3. Additional checks should not duplicate the fault.
    sensor.check(1200);
    sensor.check(1500);

    assert(log.size() == 1);

    // 4. Sensor resumes reporting.
    sensor.receiveUpdate(1600);

    assert(!sensor.isTimedOut());
    assert(log.size() == 2);

    std::cout << "[1600 ms] Sensor recovered\n";

    // 5. Sensor fails a second time.
    sensor.check(2601);

    assert(sensor.isTimedOut());
    assert(log.size() == 3);

    std::cout << "[2601 ms] Second timeout detected\n";

    // 6. Verify the recorded events.
    DiagnosticRecord event{};

    const int expectedCodes[3] = {
        SensorEvents::TIMEOUT,
        SensorEvents::RECOVERED,
        SensorEvents::TIMEOUT
    };

    const uint32_t expectedTimes[3] = {
        1001, 1600, 2601
    };

    for (std::size_t i = 0; i < 3; ++i) {
        assert(log.get(i, event));
        assert(event.code == expectedCodes[i]);
        assert(event.timestampMs == expectedTimes[i]);
    }

    // 7. Export diagnostic history.
    const std::string filename =
        "TestBench/src/diagnostic_integration.csv";

    assert(exportDiagnosticCsv(log, filename));

    std::cout << "\nDiagnostic CSV exported successfully\n";

    // 8. Read the CSV and verify its contents.
    std::ifstream file(filename);
    assert(file.is_open());

    const std::string expectedLines[4] = {
        "timestamp_ms,sensor_id,event_code,severity",
        "1001,0,1001,WARNING",
        "1600,0,1002,INFO",
        "2601,0,1001,WARNING"
    };

    std::string line;

    for (const std::string& expected : expectedLines) {
        assert(static_cast<bool>(std::getline(file, line)));

        // Handle Windows CRLF line endings.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        assert(line == expected);
    }

    assert(!std::getline(file, line));

    std::cout << "CSV contents verified\n";
    std::cout << "Events recorded: " << log.size() << "\n";

    std::cout << "\nALL INTEGRATION TESTS PASSED!\n";

    return 0;
}
