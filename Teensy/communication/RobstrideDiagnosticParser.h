/************************************************
Program: RobstrideDiagnosticParser.h
Purpose: Read-only decoding of Robstride private-CAN
         motor feedback (type 2) and faults (type 21).
Inputs: 29-bit extended CAN ID, optional eight-byte data.
Outputs: Parsed source IDs, summary flags, detailed masks.
Dependencies: <cstdint> only; no CAN hardware library.
Testing: TestBench/src/robstride_diagnostics_test.cpp
Hardware status: NOT tested with a motor or Teensy.
Version: 0.2 (prototype)
************************************************/
/************************************************
Date: 10/27/2026
Programmer: Jacob Olguin (prototype support)
Reviewer: Pending team review
Changes made: Type-2 status and type-21 fault parsing.
Next: Confirm type-21 byte order on captured frames.
Recommendations: Receive-only integration first. Do not
use this module to enable, stop, or clear motor faults.
************************************************/
#ifndef ROBSTRIDE_DIAGNOSTIC_PARSER_H
#define ROBSTRIDE_DIAGNOSTIC_PARSER_H

#include <cstdint>

namespace Robstride {

constexpr uint8_t MOTOR_FEEDBACK = 0x02;
constexpr uint8_t FAULT_FEEDBACK = 0x15;
constexpr uint32_t MAX_EXTENDED_ID = 0x1FFFFFFFu;

// Host-facing view of a CAN frame. The parser never transmits.
struct CanFrame {
    uint32_t id = 0;
    uint8_t data[8] = {};
    uint8_t length = 0;
    bool extended = false;
    bool remote = false;
};

struct FrameInfo {
    uint8_t messageType = 0;
    uint8_t motorId = 0;
    uint8_t hostId = 0;
};

// Type 2: motor -> host uses bits 15..8 for MOTOR and 7..0 for HOST.
inline bool parseDiagnosticId(uint32_t canId, bool extended,
                              FrameInfo& result) {
    if (!extended || canId > MAX_EXTENDED_ID) return false;
    const uint8_t type = static_cast<uint8_t>((canId >> 24) & 0x1Fu);
    if (type != MOTOR_FEEDBACK && type != FAULT_FEEDBACK) return false;
    FrameInfo tmp{};
    tmp.messageType = type;
    tmp.motorId = static_cast<uint8_t>((canId >> 8) & 0xFFu);
    tmp.hostId = static_cast<uint8_t>(canId & 0xFFu);
    result = tmp;
    return true;
}

// Type-2 summary flags are bits 16 through 21 of the extended ID.
struct MotorFeedbackStatus {
    uint8_t motorId = 0;
    uint8_t hostId = 0;
    uint8_t mode = 0; // 0 reset, 1 calibration, 2 run (vendor docs)
    bool undervoltage = false;
    bool overcurrent = false;
    bool overtemperature = false;
    bool encoderFault = false;
    bool overload = false;
    bool uncalibrated = false;

    // Bit 0..5 corresponds to CAN-ID bit 16..21.
    uint8_t summaryMask() const {
        return static_cast<uint8_t>(
            (undervoltage ? 0x01u : 0u) |
            (overcurrent ? 0x02u : 0u) |
            (overtemperature ? 0x04u : 0u) |
            (encoderFault ? 0x08u : 0u) |
            (overload ? 0x10u : 0u) |
            (uncalibrated ? 0x20u : 0u));
    }
};

inline bool parseMotorFeedbackStatus(uint32_t id, bool extended,
                                     MotorFeedbackStatus& result) {
    FrameInfo info{};
    if (!parseDiagnosticId(id, extended, info) ||
        info.messageType != MOTOR_FEEDBACK) return false;
    MotorFeedbackStatus out{};
    out.motorId = info.motorId;
    out.hostId = info.hostId;
    out.mode = static_cast<uint8_t>((id >> 22) & 0x03u);
    out.undervoltage = (id & (uint32_t{1} << 16)) != 0;
    out.overcurrent = (id & (uint32_t{1} << 17)) != 0;
    out.overtemperature = (id & (uint32_t{1} << 18)) != 0;
    out.encoderFault = (id & (uint32_t{1} << 19)) != 0;
    out.overload = (id & (uint32_t{1} << 20)) != 0;
    out.uncalibrated = (id & (uint32_t{1} << 21)) != 0;
    result = out;
    return true;
}

enum class FaultByteOrder { LITTLE_ENDIAN_ORDER, BIG_ENDIAN_ORDER };

struct DetailedFaultStatus {
    uint8_t motorId = 0;
    uint8_t hostId = 0;
    uint32_t faultMask = 0;
    uint32_t warningMask = 0;
    bool hasFault(uint8_t bit) const {
        return bit < 32 && (faultMask & (uint32_t{1} << bit)) != 0;
    }
    bool hasWarning(uint8_t bit) const {
        return bit < 32 && (warningMask & (uint32_t{1} << bit)) != 0;
    }
};

inline uint32_t read32(const uint8_t* bytes, FaultByteOrder byteOrder) {
    uint32_t value = 0;
    if (byteOrder == FaultByteOrder::LITTLE_ENDIAN_ORDER) {
        for (int i = 0; i < 4; ++i)
            value |= static_cast<uint32_t>(bytes[i]) << (8 * i);
    } else {
        for (int i = 0; i < 4; ++i)
            value = (value << 8) | bytes[i];
    }
    return value;
}

// Payload bytes 0..3: faults, 4..7: warnings (type 21).
// Byte order is deliberately selected by the caller: not yet hardware-verified.
inline bool parseDetailedFault(uint32_t id, bool extended,
                               const uint8_t* data, uint8_t length,
                               FaultByteOrder byteOrder,
                               DetailedFaultStatus& result) {
    FrameInfo info{};
    if (data == nullptr || length != 8 ||
        !parseDiagnosticId(id, extended, info) ||
        info.messageType != FAULT_FEEDBACK) return false;
    DetailedFaultStatus out{};
    out.motorId = info.motorId;
    out.hostId = info.hostId;
    out.faultMask = read32(data, byteOrder);
    out.warningMask = read32(data + 4, byteOrder);
    result = out;
    return true;
}

// Returns a vendor-documented label, or "Unspecified fault bit".
inline const char* detailedFaultName(uint8_t bit) {
    switch (bit) {
        case 0: return "Motor overtemperature";
        case 1: return "Driver chip fault";
        case 2: return "Undervoltage";
        case 3: return "Overvoltage";
        case 4: return "B-phase sample overcurrent";
        case 5: return "C-phase sample overcurrent";
        case 7: return "Encoder not calibrated";
        case 8: return "Hardware identification fault";
        case 9: return "Position initialization fault";
        case 14: return "Stall/overload protection";
        case 16: return "A-phase sample overcurrent";
        default: return "Unspecified fault bit";
    }
}
inline const char* warningName(uint8_t bit) {
    return bit == 0 ? "Motor overtemperature warning" : "Unspecified warning bit";
}
inline const char* summaryFaultName(uint8_t bit) {
    switch (bit) {
        case 0: return "Undervoltage (summary)";
        case 1: return "Overcurrent (summary)";
        case 2: return "Overtemperature (summary)";
        case 3: return "Magnetic encoder fault (summary)";
        case 4: return "Stall/overload (summary)";
        case 5: return "Encoder not calibrated (summary)";
        default: return "Unknown summary bit";
    }
}

} // namespace Robstride
#endif
