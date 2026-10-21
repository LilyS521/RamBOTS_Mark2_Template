
# RamBOTs Mark II - Diagnostic Software

## Overview

This software provides an initial diagnostic and sensor-monitoring
prototype for the RamBOTs Mark II quadruped robot.

The system monitors simulated sensor activity, detects missing
updates, records diagnostic events, and supports CSV export
for troubleshooting and testing.

Development target: Teensy 4.1
Current test environment: Windows, GCC/G++ with C++17

Hardware integration has not yet been performed.

## Software Components

### FaultEvent.h / FaultEvent.cpp
- Defines diagnostic event information.
- Supports INFO, WARNING, and CRITICAL severity levels.
- Includes the original event-recording functions.

### DiagnosticBuffer.h
- Implements a fixed-capacity circular event buffer.
- Stores event code, severity, timestamp, and sensor ID.
- Holds five events in the current prototype.
- Overwrites the oldest stored event when full.
- Does not require dynamically growing event storage.

### SensorMonitor.h
- Monitors sensor update timing.
- Detects sensor timeouts and recoveries.
- Supports startup timeout detection.
- Prevents repeated timeout events while a fault is active.
- Supports independently configured sensor monitors.
- Uses prototype fault codes 1001 and 1002.

### SharedSensorMonitor.h
- Provides compatibility aliases for earlier tests.
- Uses SensorMonitor.h as the underlying implementation.

### DiagnosticCsv.h
Location: TestBench/src/

- Exports retained diagnostic events into CSV files.
- Includes timestamps, sensor IDs, event codes, and severity.
- Uses standard C++ file I/O on the development computer.
- Is not yet integrated with Teensy storage hardware.

## Diagnostic Flow

Sensor update or missing update
    |
    v
SensorMonitor
    |
    v
Timeout / Recovery Detection
    |
    v
DiagnosticBuffer
    |
    v
DiagnosticCsv
    |
    v
CSV File (Laptop Testing)

## Testing

The following test programs have passed on the
Windows development computer:

1. buffer_test.cpp
   - Fixed-capacity storage
   - Circular overwrite
   - Buffer reset

2. sensor_test.cpp
   - Normal sensor operation
   - Timeout detection
   - Recovery detection
   - Duplicate event prevention
   - Timer wraparound

3. integration_test.cpp
   - Sensor monitoring and diagnostic recording
   - CSV export and content verification

4. multi_sensor_test.cpp
   - Independent sensor monitoring
   - Independent fault recovery
   - Separate CSV exports

5. shared_sensor_test.cpp
   - Shared diagnostic buffer
   - Startup timeout detection
   - Sensor identification
   - Shared CSV export

6. csv_test.cpp
   - CSV file creation
   - Export content verification

All six regression test programs passed following
sensor-monitor consolidation.

## Current Limitations

- No physical sensors have been connected.
- No Teensy hardware compilation has been verified.
- No motor-controller communication is implemented.
- Sensor IDs and event codes are provisional.
- Timeout thresholds are simulated test values.
- The diagnostic history holds only five events.
- Older events are overwritten when the buffer fills.
- Events are not automatically saved to nonvolatile
  storage during robot operation.
- CSV export is currently laptop-specific.
- No safety-critical shutdown or motor response is implemented.
- Error handling for abnormal timing, storage failures,
  and hardware conditions requires further validation.

## Planned Development

1. Review sensor and fault requirements with the team.
2. Confirm actual sensor interfaces and communication protocols.
3. Verify compilation in the Teensy development environment.
4. Determine appropriate diagnostic buffer capacity.
5. Implement hardware sensor update interfaces.
6. Select and test a persistent logging method.
7. Integrate with the main robot application.
8. Test fault behavior with physical hardware.
9. Document hardware test results.

## Safety Considerations

Diagnostic history is separate from active fault state.

Overwriting an event must not clear an active fault or
disable any required protective response.

Motor control and safety-critical behavior require
separate design review and hardware validation.
