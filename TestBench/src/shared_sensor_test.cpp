
#include <iostream>
#include <cassert>
#include <cstdint>
#include "DiagnosticCsv.h"
#include <fstream>
#include <string>

#include "../../Teensy/sensors/SharedSensorMonitor.h"

int main() {
    DiagnosticBuffer log;

    // Two sensors, one shared diagnostic history.
    // All times and IDs are simulated.
    SharedSensorMonitor sensorA(log, 1, 1000, 0);
    SharedSensorMonitor sensorB(log, 2, 1500, 0);

    // Sensor A starts reporting.
    // Sensor B never sends its first reading.
    sensorA.receiveUpdate(0);

    assert(sensorA.hasData());
    assert(!sensorB.hasData());

    // TEST 1: Sensor A times out.
    sensorA.check(1001);
    sensorB.check(1001);

    assert(sensorA.isTimedOut());
    assert(!sensorB.isTimedOut());
    assert(log.size() == 1);

    std::cout << "Test 1: Sensor A timeout PASSED\n";

    // TEST 2: Sensor B times out without ever
    // having received its first reading.
    sensorB.check(1501);

    assert(sensorB.isTimedOut());
    assert(!sensorB.hasData());
    assert(log.size() == 2);

    std::cout << "Test 2: Startup timeout PASSED\n";

    // TEST 3: Sensor A recovers.
    sensorA.receiveUpdate(1800);

    assert(!sensorA.isTimedOut());
    assert(sensorB.isTimedOut());

    std::cout << "Test 3: Independent recovery PASSED\n";

    // TEST 4: Sensor B finally sends data.
    sensorB.receiveUpdate(2200);

    assert(!sensorB.isTimedOut());
    assert(sensorB.hasData());
    assert(log.size() == 4);

    std::cout << "Test 4: Startup recovery PASSED\n";

    // TEST 5: Verify source IDs, event codes
    // and chronological event order.
    const uint16_t expectedIds[4] = {1, 2, 1, 2};

    const int expectedCodes[4] = {
        SharedSensorEvents::TIMEOUT,
        SharedSensorEvents::TIMEOUT,
        SharedSensorEvents::RECOVERED,
        SharedSensorEvents::RECOVERED
    };

    const uint32_t expectedTimes[4] = {
        1001, 1501, 1800, 2200
    };

    DiagnosticRecord event{};

    for (std::size_t i = 0; i < 4; ++i) {
        assert(log.get(i, event));
        assert(event.sensorId == expectedIds[i]);
        assert(event.code == expectedCodes[i]);
        assert(event.timestampMs == expectedTimes[i]);
    }

    std::cout << "Test 5: Shared history PASSED\n";

    // Display all events.
    std::cout << "\nShared diagnostic history:\n";

    for (std::size_t i = 0; i < log.size(); ++i) {
        if (log.get(i, event)) {
            std::cout << "[" << event.timestampMs
                      << " ms] Sensor " << event.sensorId
                      << " | Code: " << event.code
                      << "\n";
        }
    }

    std::cout << "\nAll shared sensor tests passed!\n";
    // Export events from both sensors into one CSV file.
const std::string filename =
    "TestBench/src/shared_sensor_log.csv";

assert(exportDiagnosticCsv(log, filename));

std::ifstream file(filename);
assert(file.is_open());

const std::string expectedLines[5] = {
    "timestamp_ms,sensor_id,event_code,severity",
    "1001,1,1001,WARNING",
    "1501,2,1001,WARNING",
    "1800,1,1002,INFO",
    "2200,2,1002,INFO"
};

std::string line;

for (const std::string& expected : expectedLines) {
    assert(static_cast<bool>(std::getline(file, line)));

    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    assert(line == expected);
}

assert(!std::getline(file, line));

std::cout << "Test 6: Shared CSV logging PASSED\n";

    return 0;
}
