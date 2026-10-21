
#include <iostream>
#include <cassert>
#include <cstdint>

#include "../../Teensy/sensors/SensorMonitor.h"

int main() {

    DiagnosticBuffer log;
    SensorMonitor sensor(log, 1000);

    // TEST 1: Initial state
    assert(!sensor.hasData());
    assert(!sensor.isTimedOut());

    sensor.check(5000);
    assert(log.empty());

    std::cout << "Test 1: Initial state PASSED\n";

    // TEST 2: Normal sensor operation
    sensor.receiveUpdate(0);
    sensor.check(500);
    sensor.check(1000);

    assert(sensor.hasData());
    assert(!sensor.isTimedOut());
    assert(log.empty());

    std::cout << "Test 2: Normal updates PASSED\n";

    // TEST 3: Sensor stops updating
    sensor.check(1001);

    assert(sensor.isTimedOut());
    assert(log.size() == 1);

    DiagnosticRecord event{};

    assert(log.get(0, event));
    assert(event.code == SensorEvents::TIMEOUT);
    assert(event.severity == FaultSeverity::WARNING);
    assert(event.timestampMs == 1001);

    std::cout << "Test 3: Timeout detection PASSED\n";

    // TEST 4: No duplicate warnings
    sensor.check(1200);
    sensor.check(1500);

    assert(log.size() == 1);

    std::cout << "Test 4: Duplicate prevention PASSED\n";

    // TEST 5: Sensor recovers
    sensor.receiveUpdate(1600);

    assert(!sensor.isTimedOut());
    assert(log.size() == 2);

    assert(log.get(1, event));
    assert(event.code == SensorEvents::RECOVERED);
    assert(event.severity == FaultSeverity::INFO);
    assert(event.timestampMs == 1600);

    std::cout << "Test 5: Sensor recovery PASSED\n";

    // TEST 6: Sensor fails again
    sensor.check(2600);
    assert(!sensor.isTimedOut());

    sensor.check(2601);

    assert(sensor.isTimedOut());
    assert(log.size() == 3);

    assert(log.get(2, event));
    assert(event.code == SensorEvents::TIMEOUT);

    std::cout << "Test 6: Repeated fault PASSED\n";

    // TEST 7: Timer wraparound
    DiagnosticBuffer wrapLog;
    SensorMonitor wrapSensor(wrapLog, 150);

    wrapSensor.receiveUpdate(UINT32_MAX - 100);
    wrapSensor.check(50);

    assert(wrapSensor.isTimedOut());
    assert(wrapLog.size() == 1);

    std::cout << "Test 7: Timer wraparound PASSED\n";

    // Print diagnostic history
    std::cout << "\nSensor diagnostic history:\n";

    for (std::size_t i = 0; i < log.size(); ++i) {
        if (log.get(i, event)) {
            std::cout
                << "[" << event.timestampMs << " ms] "
                << "Code: " << event.code << " | "
                << (event.code == SensorEvents::TIMEOUT
                    ? "Sensor Timeout"
                    : "Sensor Recovered")
                << "\n";
        }
    }

    std::cout << "\nAll sensor tests passed!\n";

    return 0;
}
