
#include <iostream>
#include <cassert>
#include <string>

#include "../../Teensy/sensors/SensorMonitor.h"
#include "DiagnosticCsv.h"

// Verify the event at a given position.
void verifyEvent(const DiagnosticBuffer& log,
                 std::size_t index,
                 int expectedCode,
                 uint32_t expectedTime) {
    DiagnosticRecord event{};

    assert(log.get(index, event));
    assert(event.code == expectedCode);
    assert(event.timestampMs == expectedTime);
}

int main() {
    // Each simulated sensor has its own history.
    DiagnosticBuffer logA;
    DiagnosticBuffer logB;

    // Different simulated reporting requirements.
    SensorMonitor sensorA(logA, 1000);
    SensorMonitor sensorB(logB, 500);

    // Both sensors begin operating.
    sensorA.receiveUpdate(0);
    sensorB.receiveUpdate(0);

    // At 600 ms, B has exceeded its timeout.
    // A should still be healthy.
    sensorA.check(600);
    sensorB.check(600);

    assert(!sensorA.isTimedOut());
    assert(sensorB.isTimedOut());

    verifyEvent(logB, 0, SensorEvents::TIMEOUT, 600);

    std::cout << "Test 1: Independent timeouts PASSED\n";

    // B recovers while A continues reporting.
    sensorB.receiveUpdate(700);
    sensorA.receiveUpdate(800);

    assert(!sensorB.isTimedOut());

    verifyEvent(logB, 1, SensorEvents::RECOVERED, 700);

    std::cout << "Test 2: Independent recovery PASSED\n";

    // A later times out, while B receives fresh data.
    sensorB.receiveUpdate(1500);

    sensorA.check(1801);
    sensorB.check(1801);

    assert(sensorA.isTimedOut());
    assert(!sensorB.isTimedOut());

    verifyEvent(logA, 0, SensorEvents::TIMEOUT, 1801);

    std::cout << "Test 3: Fault isolation PASSED\n";

    // Confirm the two logs remain independent.
    assert(logA.size() == 1);
    assert(logB.size() == 2);

    // Export separate histories for each sensor.
    const std::string fileA =
        "TestBench/src/sensor_A_log.csv";
    const std::string fileB =
        "TestBench/src/sensor_B_log.csv";

    assert(exportDiagnosticCsv(logA, fileA));
    assert(exportDiagnosticCsv(logB, fileB));

    std::cout << "Test 4: CSV exports PASSED\n";

    std::cout << "\nSensor A events: " << logA.size();
    std::cout << "\nSensor B events: " << logB.size();

    std::cout << "\n\nAll multi-sensor tests passed!\n";

    return 0;
}
