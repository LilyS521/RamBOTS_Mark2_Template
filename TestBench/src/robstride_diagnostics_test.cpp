/************************************************
Program: robstride_diagnostics_test.cpp
Purpose: Verify read-only Robstride type-2/type-21 CAN
         parsing and per-motor fault transition logging.
Inputs: Synthetic 29-bit CAN frames, simulated timestamps.
Outputs: PASS or assertion failure (PC only).
Dependencies: RobstrideMotorDiagnostics.h; existing
              DiagnosticBuffer.h and FaultEvent.h.
Testing: C++17/G++ on laptop; hardware not tested.
Version: 0.1
************************************************/
#include <cassert>
#include <cstdint>
#include <iostream>
#include "../../Teensy/communication/RobstrideMotorDiagnostics.h"

using namespace Robstride;

static uint32_t makeId(uint8_t type, uint8_t motor, uint8_t host = 1,
                       uint8_t highByte = 0) {
    return (static_cast<uint32_t>(type) << 24) |
           (static_cast<uint32_t>(highByte) << 16) |
           (static_cast<uint32_t>(motor) << 8) | host;
}
static CanFrame makeFeedback(uint8_t motor, uint8_t faultMask = 0,
                             uint8_t host = 1) {
    CanFrame frame{};
    frame.extended = true;
    frame.length = 8;
    frame.id = makeId(MOTOR_FEEDBACK, motor, host, faultMask);
    return frame;
}
static CanFrame makeDetailed(uint8_t motor, uint32_t faults,
                             uint32_t warnings = 0, uint8_t host = 1) {
    CanFrame frame{};
    frame.extended = true;
    frame.length = 8;
    frame.id = makeId(FAULT_FEEDBACK, motor, host);
    // Synthetic LITTLE-ENDIAN payload, NOT hardware-confirmed byte order.
    for (int i = 0; i < 4; ++i) {
        frame.data[i] = static_cast<uint8_t>(faults >> (8 * i));
        frame.data[i + 4] = static_cast<uint8_t>(warnings >> (8 * i));
    }
    return frame;
}
static DiagnosticRecord newest(const DiagnosticBuffer& log) {
    DiagnosticRecord event{};
    assert(log.size() != 0);
    assert(log.get(log.size() - 1, event));
    return event;
}
static void expectNewest(const DiagnosticBuffer& log, int code,
                         uint16_t motorId, uint32_t timeMs) {
    const DiagnosticRecord event = newest(log);
    assert(event.code == code);
    assert(event.sensorId == motorId);
    assert(event.timestampMs == timeMs);
}

int main() {
    // 1. CAN ID decoding and malformed frames.
    FrameInfo info{};
    assert(parseDiagnosticId(makeId(FAULT_FEEDBACK, 3), true, info));
    assert(info.messageType == FAULT_FEEDBACK && info.motorId == 3 && info.hostId == 1);
    assert(!parseDiagnosticId(0x301, false, info));
    assert(!parseDiagnosticId(0x20000000, true, info));
    assert(!parseDiagnosticId(makeId(3, 3), true, info));
    std::cout << "Test 1: CAN identifier validation PASSED\n";

    // 2. Type-2 summary decoding: undervoltage + overtemp + RUN mode.
    MotorFeedbackStatus summary{};
    const uint32_t summaryId = makeId(MOTOR_FEEDBACK, 3, 1,
                                     static_cast<uint8_t>((1u << 0) | (1u << 2) | (2u << 6)));
    assert(parseMotorFeedbackStatus(summaryId, true, summary));
    assert(summary.motorId == 3 && summary.hostId == 1);
    assert(summary.mode == 2 && summary.undervoltage && summary.overtemperature);
    assert(!summary.overcurrent && summary.summaryMask() == 0x05);
    std::cout << "Test 2: Type-2 summary decoding PASSED\n";

    // 3. Type-21 detailed bitfields and explicit byte ordering.
    DetailedFaultStatus detail{};
    const CanFrame fault = makeDetailed(3, (1u << 2) | (1u << 14), 1u);
    assert(parseDetailedFault(fault.id, true, fault.data, 8,
                             FaultByteOrder::LITTLE_ENDIAN_ORDER, detail));
    assert(detail.motorId == 3 && detail.faultMask == 0x4004u);
    assert(detail.hasFault(2) && detail.hasFault(14) && detail.hasWarning(0));
    assert(!detail.hasFault(3) && !detail.hasWarning(1));
    const uint8_t big[8] = {0, 0, 0x40, 0x04, 0, 0, 0, 1};
    assert(parseDetailedFault(fault.id, true, big, 8,
                             FaultByteOrder::BIG_ENDIAN_ORDER, detail));
    assert(detail.faultMask == 0x4004u && detail.warningMask == 1);
    assert(!parseDetailedFault(fault.id, true, big, 7,
                              FaultByteOrder::BIG_ENDIAN_ORDER, detail));
    assert(!parseDetailedFault(fault.id, true, nullptr, 8,
                              FaultByteOrder::BIG_ENDIAN_ORDER, detail));
    std::cout << "Test 3: Detailed fault and byte order PASSED\n";

    // 4. Register and isolate motor IDs; no dynamic motor creation.
    DiagnosticBuffer history;
    MotorDiagnostics monitor(history, FaultByteOrder::LITTLE_ENDIAN_ORDER);
    assert(monitor.registerMotor(3, 0, 1000));
    assert(monitor.registerMotor(5, 0, 500));
    assert(!monitor.registerMotor(3, 0, 1000));
    assert(!monitor.registerMotor(0, 0, 1000));
    assert(monitor.registeredCount() == 2);
    assert(monitor.receive(makeDetailed(8, 1), 20) == ProcessResult::UNREGISTERED_MOTOR);
    assert(history.empty());
    std::cout << "Test 4: Motor registration PASSED\n";

    // 5. New detailed fault: logs each changed bit exactly once.
    assert(monitor.receive(makeDetailed(3, 1u << 2), 100) == ProcessResult::ACCEPTED);
    assert(history.size() == 1);
    expectNewest(history, EventCode::DETAIL_ACTIVE + 2, 3, 100);
    assert(monitor.receive(makeDetailed(3, 1u << 2), 110) == ProcessResult::ACCEPTED);
    assert(history.size() == 1); // duplicate suppressed
    MotorSnapshot snap{};
    assert(monitor.snapshot(3, snap) && snap.detailedFaultMask == (1u << 2));
    assert(!monitor.snapshot(8, snap));
    std::cout << "Test 5: Fault edge detection and dedup PASSED\n";

    // 6. Independent motor / warning transition / fault cleared.
    assert(monitor.receive(makeDetailed(5, 0, 1), 200) == ProcessResult::ACCEPTED);
    assert(history.size() == 2);
    expectNewest(history, EventCode::WARNING_ACTIVE, 5, 200);
    assert(monitor.receive(makeDetailed(3, 0), 300) == ProcessResult::ACCEPTED);
    assert(history.size() == 3);
    expectNewest(history, EventCode::DETAIL_CLEARED + 2, 3, 300);
    assert(monitor.receive(makeDetailed(5, 0, 0), 400) == ProcessResult::ACCEPTED);
    expectNewest(history, EventCode::WARNING_CLEARED, 5, 400);
    assert(monitor.snapshot(5, snap) && snap.warningMask == 0);
    std::cout << "Test 6: Per-motor warning and clearing PASSED\n";

    // 7. Type-2 summary faults are tracked separately from detailed type-21 bits.
    assert(monitor.receive(makeFeedback(3, 1u << 0), 450) == ProcessResult::ACCEPTED);
    expectNewest(history, EventCode::SUMMARY_ACTIVE, 3, 450);
    assert(monitor.receive(makeFeedback(3, 1u << 0), 460) == ProcessResult::ACCEPTED);
    assert(monitor.receive(makeFeedback(3, 0), 470) == ProcessResult::ACCEPTED);
    expectNewest(history, EventCode::SUMMARY_CLEARED, 3, 470);
    std::cout << "Test 7: Summary faults and recovery PASSED\n";

    // 8. Invalid data must not refresh the motor's last received timestamp.
    assert(monitor.snapshot(3, snap) && snap.lastRxMs == 470);
    CanFrame malformed = makeFeedback(3);
    malformed.length = 7;
    assert(monitor.receive(malformed, 999) == ProcessResult::INVALID_FRAME);
    assert(monitor.snapshot(3, snap) && snap.lastRxMs == 470);
    malformed = makeFeedback(3);
    malformed.extended = false;
    assert(monitor.receive(malformed, 999) == ProcessResult::INVALID_FRAME);
    assert(monitor.snapshot(3, snap) && snap.lastRxMs == 470);
    std::cout << "Test 8: Malformed frame rejection PASSED\n";

    // 9. Communication loss independent for each motor; no duplicate event.
    history.clear();
    monitor.checkTimeouts(971); // Motor 5: 971-400 > 500; Motor 3: 971-470 < 1000.
    expectNewest(history, EventCode::COMM_TIMEOUT, 5, 971);
    assert(history.size() == 1);
    monitor.checkTimeouts(1200);
    assert(history.size() == 1);
    assert(monitor.snapshot(5, snap) && snap.communicationTimedOut);
    assert(monitor.receive(makeFeedback(5), 1250) == ProcessResult::ACCEPTED);
    expectNewest(history, EventCode::COMM_RECOVERED, 5, 1250);
    std::cout << "Test 9: Independent communication monitoring PASSED\n";

    // 10. A registered motor with no data can time out from startup.
    DiagnosticBuffer startupLog;
    MotorDiagnostics startupMonitor(startupLog, FaultByteOrder::LITTLE_ENDIAN_ORDER);
    assert(startupMonitor.registerMotor(9, 0, 200, true));
    startupMonitor.checkTimeouts(200);
    assert(startupLog.empty());
    startupMonitor.checkTimeouts(201);
    expectNewest(startupLog, EventCode::COMM_TIMEOUT, 9, 201);
    assert(startupMonitor.receive(makeFeedback(9), 250) == ProcessResult::ACCEPTED);
    expectNewest(startupLog, EventCode::COMM_RECOVERED, 9, 250);
    std::cout << "Test 10: Startup timeout and recovery PASSED\n";

    // 11. Counter wraparound arithmetic (uint32_t monotonic timestamps).
    DiagnosticBuffer wrapLog;
    MotorDiagnostics wrapMonitor(wrapLog, FaultByteOrder::LITTLE_ENDIAN_ORDER);
    assert(wrapMonitor.registerMotor(11, UINT32_MAX - 100u, 150, true));
    wrapMonitor.checkTimeouts(49); // elapsed 150
    assert(wrapLog.empty());
    wrapMonitor.checkTimeouts(50); // elapsed 151
    expectNewest(wrapLog, EventCode::COMM_TIMEOUT, 11, 50);
    std::cout << "Test 11: Timestamp wraparound PASSED\n";

    // 12. Active motor state persists even when 5-event history overwrites.
    DiagnosticBuffer smallLog;
    MotorDiagnostics smallMonitor(smallLog, FaultByteOrder::LITTLE_ENDIAN_ORDER);
    assert(smallMonitor.registerMotor(1, 0, 1000));
    assert(smallMonitor.receive(makeDetailed(1, 0x7Fu), 10) == ProcessResult::ACCEPTED);
    assert(smallLog.size() == DiagnosticBuffer::CAPACITY);
    assert(smallMonitor.snapshot(1, snap) && snap.detailedFaultMask == 0x7Fu);
    std::cout << "Test 12: Bounded log / persistent active state PASSED\n";

    std::cout << "\nAll 12 Robstride diagnostic tests passed!\n";
    return 0;
}
