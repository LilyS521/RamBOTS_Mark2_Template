
#ifndef FAULT_EVENT_H
#define FAULT_EVENT_H

#include <string>
#include <vector>

// Severity classification for diagnostic events
enum class FaultSeverity {
    INFO,
    WARNING,
    CRITICAL
};

// Information associated with a diagnostic event
struct FaultEvent {
    int code;
    std::string name;
    FaultSeverity severity;
    unsigned long timestampMs;
};

// Convert a severity level to readable text
std::string severityToString(FaultSeverity severity);

// Add a diagnostic event to an existing collection
void logEvent(std::vector<FaultEvent>& events,
              int code,
              const std::string& name,
              FaultSeverity severity,
              unsigned long timestampMs);

#endif
