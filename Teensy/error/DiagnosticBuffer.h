
#ifndef DIAGNOSTIC_BUFFER_H
#define DIAGNOSTIC_BUFFER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include "FaultEvent.h"

// One diagnostic event with source identification.
struct DiagnosticRecord {
    int code;
    FaultSeverity severity;
    uint32_t timestampMs;
    uint16_t sensorId;
};

// Fixed-capacity diagnostic history.
class DiagnosticBuffer {
public:
    static constexpr std::size_t CAPACITY = 5;

    // The optional sensorId preserves older calls.
    // Zero indicates an unspecified event source.
    void add(int code,
             FaultSeverity severity,
             uint32_t timestampMs,
             uint16_t sensorId = 0) {

        records[nextWrite] = {
            code,
            severity,
            timestampMs,
            sensorId
        };

        nextWrite = (nextWrite + 1) % CAPACITY;

        if (eventCount < CAPACITY) {
            ++eventCount;
        }
    }

    std::size_t size() const {
        return eventCount;
    }

    bool empty() const {
        return eventCount == 0;
    }

    bool full() const {
        return eventCount == CAPACITY;
    }

    // Index zero returns the oldest retained event.
    bool get(std::size_t index,
             DiagnosticRecord& result) const {

        if (index >= eventCount) {
            return false;
        }

        std::size_t oldest =
            (nextWrite + CAPACITY - eventCount)
            % CAPACITY;

        std::size_t position =
            (oldest + index) % CAPACITY;

        result = records[position];
        return true;
    }

    void clear() {
        nextWrite = 0;
        eventCount = 0;
    }

private:
    std::array<DiagnosticRecord, CAPACITY> records{};

    std::size_t nextWrite = 0;
    std::size_t eventCount = 0;
};

#endif
