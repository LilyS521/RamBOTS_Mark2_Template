
#ifndef DIAGNOSTIC_CSV_H
#define DIAGNOSTIC_CSV_H

#include <fstream>
#include <string>
#include "../../Teensy/error/DiagnosticBuffer.h"

// Convert fault severity to a readable label.
inline const char* csvSeverity(FaultSeverity severity) {
    switch (severity) {
        case FaultSeverity::INFO: return "INFO";
        case FaultSeverity::WARNING: return "WARNING";
        case FaultSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// Export buffered events into a CSV file.
// Returns true only when the file was written successfully.
inline bool exportDiagnosticCsv(
    const DiagnosticBuffer& buffer,
    const std::string& filename
) {
    std::ofstream file(filename, std::ios::out | std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    file << "timestamp_ms,sensor_id,event_code,severity\n";

    DiagnosticRecord event{};

    for (std::size_t i = 0; i < buffer.size(); ++i) {
        if (!buffer.get(i, event)) {
            return false;
    }

    file << event.timestampMs << ","
         << event.sensorId << ","
         << event.code << ","
         << csvSeverity(event.severity) << "\n";
}

    file.flush();
    return file.good();
}

#endif
