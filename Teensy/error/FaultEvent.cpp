
#include "FaultEvent.h"

// Convert severity classification to readable text
std::string severityToString(FaultSeverity severity) {
    switch (severity) {
        case FaultSeverity::INFO:
            return "INFO";

        case FaultSeverity::WARNING:
            return "WARNING";

        case FaultSeverity::CRITICAL:
            return "CRITICAL";

        default:
            return "UNKNOWN";
    }
}

// Add a new diagnostic event to the collection
void logEvent(std::vector<FaultEvent>& events,
              int code,
              const std::string& name,
              FaultSeverity severity,
              unsigned long timestampMs) {

    FaultEvent newEvent = {
        code,
        name,
        severity,
        timestampMs
    };

    events.push_back(newEvent);
}
