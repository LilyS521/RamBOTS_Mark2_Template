
#ifndef SENSOR_MONITOR_H
#define SENSOR_MONITOR_H

#include <cstdint>
#include "../error/DiagnosticBuffer.h"

// Prototype event codes, pending team approval.
namespace SensorEvents {
    constexpr int TIMEOUT = 1001;
    constexpr int RECOVERED = 1002;
}

// Monitors one sensor and records timeout/recovery events.
// Multiple instances can share one DiagnosticBuffer.
class SensorMonitor {
public:
    // Legacy mode: monitoring begins after first update.
    SensorMonitor(DiagnosticBuffer& log,
                  uint32_t timeoutMs)
        : diagnosticLog(log),
          sensorId(0),
          timeout(timeoutMs),
          lastUpdateMs(0),
          startupMonitoring(false) {}

    // Sensor-specific mode: monitor from startup time.
    SensorMonitor(DiagnosticBuffer& log,
                  uint16_t id,
                  uint32_t timeoutMs,
                  uint32_t startupMs)
        : diagnosticLog(log),
          sensorId(id),
          timeout(timeoutMs),
          lastUpdateMs(startupMs),
          startupMonitoring(true) {}

    // Call when a valid sensor update arrives.
    void receiveUpdate(uint32_t currentTimeMs) {
        if (timedOut) {
            diagnosticLog.add(
                SensorEvents::RECOVERED,
                FaultSeverity::INFO,
                currentTimeMs,
                sensorId
            );
        }

        lastUpdateMs = currentTimeMs;
        receivedData = true;
        timedOut = false;
    }

    // Call periodically to detect missing updates.
    void check(uint32_t currentTimeMs) {
        if (timedOut) {
            return;
        }

        // Legacy mode waits for the first reading.
        if (!startupMonitoring && !receivedData) {
            return;
        }

        // Unsigned subtraction handles 32-bit wraparound.
        uint32_t elapsed =
            currentTimeMs - lastUpdateMs;

        if (elapsed > timeout) {
            timedOut = true;

            diagnosticLog.add(
                SensorEvents::TIMEOUT,
                FaultSeverity::WARNING,
                currentTimeMs,
                sensorId
            );
        }
    }

    bool isTimedOut() const {
        return timedOut;
    }

    bool hasData() const {
        return receivedData;
    }

    uint16_t getSensorId() const {
        return sensorId;
    }

private:
    DiagnosticBuffer& diagnosticLog;

    uint16_t sensorId;
    uint32_t timeout;
    uint32_t lastUpdateMs;

    bool startupMonitoring;
    bool receivedData = false;
    bool timedOut = false;
};

#endif
