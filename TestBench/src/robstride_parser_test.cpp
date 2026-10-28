
/************************************************
Program: robstride_parser_test.cpp
Purpose:
Test Robstride CAN diagnostic identifier parsing
using simulated CAN frames on a Windows computer.

Inputs:
- Simulated 29-bit CAN identifiers
- Valid and invalid frame types

Outputs:
- PASS/FAIL results for parser tests

Dependencies:
- RobstrideDiagnosticParser.h
- Standard C++ libraries

Testing:
- GCC/G++ compiler with C++17
- No physical motor hardware required

Version: 1.0
*************************************************/

/************************************************
Date: 10/27/2026
Programmer: Jacob Olguin
Reviewer:
Changes made:
- Added tests for motor and fault feedback IDs.
- Added invalid-frame rejection tests.

Next:
- Add detailed motor fault-bit tests.
- Test multiple motor IDs.

Recommendations:
- Compare results with actual motor CAN traffic.
*************************************************/

#include <iostream>
#include <cassert>

#include "../../Teensy/communication/RobstrideDiagnosticParser.h"

int main() {
    Robstride::FrameInfo frame;

    // Simulated type-21 diagnostic frame:
    // Message type 0x15, Motor ID 3, Host ID 1.
    uint32_t faultId = 0x15000301;

    assert(Robstride::parseDiagnosticId(
        faultId, true, frame));

    assert(frame.messageType == 0x15);
    assert(frame.motorId == 3);
    assert(frame.hostId == 1);

    std::cout << "Test 1: Fault frame ID PASSED\n";

    // Simulated type-2 motor feedback frame.
    uint32_t feedbackId = 0x02000501;

    assert(Robstride::parseDiagnosticId(
        feedbackId, true, frame));

    assert(frame.messageType == 0x02);
    assert(frame.motorId == 5);
    assert(frame.hostId == 1);

    std::cout << "Test 2: Motor feedback ID PASSED\n";

    // Reject standard 11-bit CAN frames.
    assert(!Robstride::parseDiagnosticId(
        0x301, false, frame));

    std::cout << "Test 3: Standard frame rejection PASSED\n";

    // Reject unsupported message types.
    assert(!Robstride::parseDiagnosticId(
        0x03000301, true, frame));

    // Reject IDs larger than 29 bits.
    assert(!Robstride::parseDiagnosticId(
        0x20000000, true, frame));

    std::cout << "Test 4: Invalid frame rejection PASSED\n";

    std::cout << "\nAll Robstride CAN ID tests passed!\n";
        
    // TEST 5: Simulate Motor 3 undervoltage.
    Robstride::MotorFeedbackStatus status{};

    // Build a type-2 CAN ID with:
    // motor ID 3, host ID 1, RUN mode (2),
    // and undervoltage fault (bit 16).
    uint32_t undervoltageId =
        (2UL << 24) |
        (2UL << 22) |
        (1UL << 16) |
        (3UL << 8) |
        1UL;

    assert(Robstride::parseMotorFeedbackStatus(
        undervoltageId, true, status));

    assert(status.motorId == 3);
    assert(status.hostId == 1);
    assert(status.mode == 2);
    assert(status.undervoltage);
    assert(!status.overcurrent);
    assert(!status.overtemperature);
    assert(!status.encoderFault);
    assert(!status.overload);
    assert(!status.uncalibrated);

    std::cout << "Test 5: Undervoltage decoding PASSED\n";

    // TEST 6: Simulate multiple simultaneous faults.
    uint32_t multipleFaultId =
        (2UL << 24) |
        (1UL << 18) |
        (1UL << 20) |
        (5UL << 8) |
        1UL;

    assert(Robstride::parseMotorFeedbackStatus(
        multipleFaultId, true, status));

    assert(status.motorId == 5);
    assert(status.overtemperature);
    assert(status.overload);
    assert(!status.undervoltage);
    assert(status.mode == 0);

    std::cout << "Test 6: Multiple fault decoding PASSED\n";

    // TEST 7: Reject a type-21 identifier.
    Robstride::MotorFeedbackStatus rejected{};

    assert(!Robstride::parseMotorFeedbackStatus(
        0x15000301, true, rejected));

    std::cout << "Test 7: Wrong message type rejected PASSED\n";

        return 0;
}
