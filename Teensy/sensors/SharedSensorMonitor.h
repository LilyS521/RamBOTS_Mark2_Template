
#ifndef SHARED_SENSOR_MONITOR_H
#define SHARED_SENSOR_MONITOR_H

#include "SensorMonitor.h"

// Compatibility aliases for existing tests.
// New code should use SensorMonitor directly.
using SharedSensorMonitor = SensorMonitor;

namespace SharedSensorEvents {
    constexpr int TIMEOUT = SensorEvents::TIMEOUT;
    constexpr int RECOVERED = SensorEvents::RECOVERED;
}

#endif
