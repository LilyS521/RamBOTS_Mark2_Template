
#include <iostream>
#include <cassert>
#include "../../Teensy/error/DiagnosticBuffer.h"

int main() {
    DiagnosticBuffer buffer;
    DiagnosticRecord event{};

    // TEST 1: Initial state
    assert(buffer.empty());
    assert(buffer.size() == 0);
    assert(!buffer.full());
    assert(!buffer.get(0, event));

    // TEST 2: Store three events
    buffer.add(0, FaultSeverity::INFO, 1000);
    buffer.add(101, FaultSeverity::WARNING, 2500);
    buffer.add(201, FaultSeverity::CRITICAL, 3200);

    assert(buffer.size() == 3);

    assert(buffer.get(0, event));
    assert(event.code == 0);
    assert(event.timestampMs == 1000);

    assert(buffer.get(1, event));
    assert(event.code == 101);
    assert(event.severity == FaultSeverity::WARNING);

    assert(buffer.get(2, event));
    assert(event.code == 201);
    assert(event.severity == FaultSeverity::CRITICAL);

    std::cout << "Test 1: Event storage PASSED\n";

    // TEST 3: Fill the buffer
    buffer.add(102, FaultSeverity::WARNING, 4000);
    buffer.add(202, FaultSeverity::INFO, 5000);

    assert(buffer.full());
    assert(buffer.size() == 5);

    std::cout << "Test 2: Full buffer PASSED\n";

    // TEST 4: Overwrite the oldest events
    buffer.add(103, FaultSeverity::WARNING, 6000);
    buffer.add(203, FaultSeverity::CRITICAL, 7000);

    assert(buffer.size() == 5);

    // Oldest two events should be gone.
    const int expectedCodes[5] = {
        201, 102, 202, 103, 203
    };

    for (std::size_t i = 0; i < buffer.size(); ++i) {
        assert(buffer.get(i, event));
        assert(event.code == expectedCodes[i]);
    }

    assert(!buffer.get(5, event));

    std::cout << "Test 3: Circular overwrite PASSED\n";

    // Print retained diagnostic events
    std::cout << "\nStored diagnostic history:\n";

    for (std::size_t i = 0; i < buffer.size(); ++i) {
        if (buffer.get(i, event)) {
            std::cout << "[" << event.timestampMs
                      << " ms] Code: "
                      << event.code << "\n";
        }
    }

    // TEST 5: Clear the buffer
    buffer.clear();

    assert(buffer.empty());
    assert(buffer.size() == 0);
    assert(!buffer.get(0, event));

    std::cout << "\nTest 4: Buffer reset PASSED\n";
    std::cout << "All circular buffer tests passed!\n";

    return 0;
}
