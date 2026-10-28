/************************************************
Program: RobstrideMotorDiagnostics.h
Purpose: Maintain per-motor diagnostic state and append
         changes to the existing shared DiagnosticBuffer.
Inputs: Read-only incoming CAN frames, monotonic ms ticks.
Outputs: Timestamped per-motor events and status snapshots.
Dependencies: RobstrideDiagnosticParser.h, DiagnosticBuffer.h
Testing: TestBench/src/robstride_diagnostics_test.cpp
Hardware status: Synthetic frames ONLY. No safety outputs.
Version: 0.1 (prototype)
************************************************/
/************************************************
Date: 10/27/2026
Programmer: Jacob Olguin (prototype support)
Reviewer: Pending team review
Changes made: Multi-motor fault transitions and timeouts.
Next: Confirm event codes, bus timing, and real fault frames.
Recommendations: Never use overwritten log entries as
active fault state or as a safety-interlock input.
************************************************/
#ifndef ROBSTRIDE_MOTOR_DIAGNOSTICS_H
#define ROBSTRIDE_MOTOR_DIAGNOSTICS_H

#include <cstddef>
#include <cstdint>
#include "RobstrideDiagnosticParser.h"
#include "../error/DiagnosticBuffer.h"

namespace Robstride {

// PROVISIONAL event-code scheme; team review required.
// Detailed faults: 3000+bit active, 3500+bit recovered.
// Detailed warnings: 3100+bit active, 3600+bit recovered.
// Summary flags: 3200+bit active, 3700+bit recovered.
// Communication: 3900 timeout, 3901 restored.
namespace EventCode {
    constexpr int DETAIL_ACTIVE = 3000;
    constexpr int WARNING_ACTIVE = 3100;
    constexpr int SUMMARY_ACTIVE = 3200;
    constexpr int DETAIL_CLEARED = 3500;
    constexpr int WARNING_CLEARED = 3600;
    constexpr int SUMMARY_CLEARED = 3700;
    constexpr int COMM_TIMEOUT = 3900;
    constexpr int COMM_RECOVERED = 3901;
}

enum class ProcessResult {
    ACCEPTED,
    NOT_DIAGNOSTIC,
    INVALID_FRAME,
    UNREGISTERED_MOTOR
};

struct MotorSnapshot {
    bool registered = false;
    bool hasReceived = false;
    bool communicationTimedOut = false;
    uint8_t motorId = 0;
    uint32_t lastRxMs = 0;
    uint8_t summaryFaultMask = 0;
    uint32_t detailedFaultMask = 0;
    uint32_t warningMask = 0;
};

// Supports up to sixteen configured motors, with no dynamic allocation.
// Not thread-safe, not ISR-safe; call from a single application task.
class MotorDiagnostics {
public:
    static constexpr std::size_t MAX_MOTORS = 16;

    MotorDiagnostics(DiagnosticBuffer& history,
                     FaultByteOrder payloadOrder)
        : log_(history), byteOrder_(payloadOrder) {}

    // Sensor id in events == motorId. 0 is reserved for unassigned sources.
    // Comm monitoring begins at startupMs if monitorFromStartup is true;
    // otherwise after the first valid received diagnostic frame.
    bool registerMotor(uint8_t motorId, uint32_t startupMs,
                       uint32_t timeoutMs, bool monitorFromStartup = true) {
        if (motorId == 0 || timeoutMs == 0 || count_ >= MAX_MOTORS ||
            findMotor(motorId) != nullptr) return false;
        MotorState& m = motors_[count_++];
        m = MotorState{};
        m.motorId = motorId;
        m.lastRxMs = startupMs;
        m.timeoutMs = timeoutMs;
        m.fromStartup = monitorFromStartup;
        return true;
    }

    ProcessResult receive(const CanFrame& frame, uint32_t nowMs) {
        FrameInfo info{};
        if (!frame.extended || frame.id > MAX_EXTENDED_ID || frame.remote)
            return ProcessResult::INVALID_FRAME;
        if (!parseDiagnosticId(frame.id, frame.extended, info))
            return ProcessResult::NOT_DIAGNOSTIC;
        if (frame.length != 8) return ProcessResult::INVALID_FRAME;

        MotorState* motor = findMotor(info.motorId);
        if (motor == nullptr) return ProcessResult::UNREGISTERED_MOTOR;

        // Decode first; invalid frames must never refresh communication state.
        MotorFeedbackStatus summary{};
        DetailedFaultStatus detail{};
        if (info.messageType == MOTOR_FEEDBACK) {
            if (!parseMotorFeedbackStatus(frame.id, true, summary))
                return ProcessResult::INVALID_FRAME;
        } else if (!parseDetailedFault(frame.id, true, frame.data,
                                       frame.length, byteOrder_, detail)) {
            return ProcessResult::INVALID_FRAME;
        }

        motor->lastRxMs = nowMs;
        motor->hasReceived = true;
        if (motor->commTimedOut) {
            motor->commTimedOut = false;
            log_.add(EventCode::COMM_RECOVERED, FaultSeverity::INFO,
                     nowMs, motor->motorId);
        }
        if (info.messageType == MOTOR_FEEDBACK) {
            emitTransitions(motor->summaryMask, summary.summaryMask(),
                            6, EventCode::SUMMARY_ACTIVE,
                            EventCode::SUMMARY_CLEARED,
                            motor->motorId, nowMs);
            motor->summaryMask = summary.summaryMask();
        } else {
            emitTransitions(motor->faultMask, detail.faultMask,
                            32, EventCode::DETAIL_ACTIVE,
                            EventCode::DETAIL_CLEARED,
                            motor->motorId, nowMs);
            emitTransitions(motor->warningMask, detail.warningMask,
                            32, EventCode::WARNING_ACTIVE,
                            EventCode::WARNING_CLEARED,
                            motor->motorId, nowMs);
            motor->faultMask = detail.faultMask;
            motor->warningMask = detail.warningMask;
        }
        return ProcessResult::ACCEPTED;
    }

    // Invoke periodically with the same uint32_t monotonic clock as receive.
    // An exact timeout is healthy; fault once elapsed > timeoutMs.
    void checkTimeouts(uint32_t nowMs) {
        for (std::size_t i = 0; i < count_; ++i) {
            MotorState& motor = motors_[i];
            if ((!motor.fromStartup && !motor.hasReceived) ||
                motor.commTimedOut) continue;
            if (static_cast<uint32_t>(nowMs - motor.lastRxMs) >
                motor.timeoutMs) {
                motor.commTimedOut = true;
                log_.add(EventCode::COMM_TIMEOUT, FaultSeverity::WARNING,
                         nowMs, motor.motorId);
            }
        }
    }

    bool snapshot(uint8_t motorId, MotorSnapshot& out) const {
        const MotorState* m = findMotor(motorId);
        if (m == nullptr) return false;
        MotorSnapshot temp{};
        temp.registered = true;
        temp.hasReceived = m->hasReceived;
        temp.communicationTimedOut = m->commTimedOut;
        temp.motorId = m->motorId;
        temp.lastRxMs = m->lastRxMs;
        temp.summaryFaultMask = m->summaryMask;
        temp.detailedFaultMask = m->faultMask;
        temp.warningMask = m->warningMask;
        out = temp;
        return true;
    }

    std::size_t registeredCount() const { return count_; }

private:
    struct MotorState {
        uint8_t motorId = 0;
        uint32_t lastRxMs = 0;
        uint32_t timeoutMs = 0;
        uint32_t faultMask = 0;
        uint32_t warningMask = 0;
        uint8_t summaryMask = 0;
        bool fromStartup = false;
        bool hasReceived = false;
        bool commTimedOut = false;
    };

    MotorState* findMotor(uint8_t motorId) {
        for (std::size_t i = 0; i < count_; ++i)
            if (motors_[i].motorId == motorId) return &motors_[i];
        return nullptr;
    }
    const MotorState* findMotor(uint8_t motorId) const {
        for (std::size_t i = 0; i < count_; ++i)
            if (motors_[i].motorId == motorId) return &motors_[i];
        return nullptr;
    }

    void emitTransitions(uint32_t oldBits, uint32_t newBits,
                         uint8_t bitCount, int activeBase, int clearedBase,
                         uint8_t motorId, uint32_t timestampMs) {
        const uint32_t changed = oldBits ^ newBits;
        for (uint8_t bit = 0; bit < bitCount; ++bit) {
            const uint32_t mask = uint32_t{1} << bit;
            if (!(changed & mask)) continue;
            const bool active = (newBits & mask) != 0;
            log_.add((active ? activeBase : clearedBase) + bit,
                     active ? FaultSeverity::WARNING : FaultSeverity::INFO,
                     timestampMs, motorId);
        }
    }

    DiagnosticBuffer& log_;
    FaultByteOrder byteOrder_;
    MotorState motors_[MAX_MOTORS]{};
    std::size_t count_ = 0;
};

} // namespace Robstride
#endif
