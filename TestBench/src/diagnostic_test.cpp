
#include <iostream>
#include <vector>
#include "../../Teensy/error/FaultEvent.h"
#include <cassert>

int main() {
    // Create an empty diagnostic event collection
    std::vector<FaultEvent> events;
    // Verify the diagnostic log starts empty
        assert(events.empty());

    // Simulate diagnostic events
    logEvent(events, 0, "Controller Started",
             FaultSeverity::INFO, 1000);

    logEvent(events, 101, "Sensor Timeout",
             FaultSeverity::WARNING, 2500);

    logEvent(events, 201, "Communication Lost",
             FaultSeverity::CRITICAL, 3200);
    // Verify the sensor timeout event
        assert(events[1].code == 101);
        assert(events[1].name == "Sensor Timeout");
        assert(events[1].severity == FaultSeverity::WARNING);
        assert(events[1].timestampMs == 2500);

    // Verify the critical communication event
        assert(events[2].code == 201);
        assert(events[2].severity == FaultSeverity::CRITICAL);
        assert(events[2].timestampMs == 3200);

    std::cout << "All diagnostic tests passed!\n\n";

    std::cout << "RamBOTs Mark II - Diagnostic Log\n\n";

    // Display each recorded event
    for (const FaultEvent& event : events) {
        std::cout << "[" << event.timestampMs << " ms] "
                  << severityToString(event.severity)
                  << " | Code: " << event.code
                  << " | " << event.name << "\n";
    }

    return 0;
}
