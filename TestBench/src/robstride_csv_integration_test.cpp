/************************************************
Program: robstride_csv_integration_test.cpp
Purpose: Demonstrate simulated motor CAN diagnostic frames
         passing into the existing diagnostic CSV exporter.
Inputs: Synthetic type-21 motor faults and recoveries.
Outputs: Verified CSV with per-motor event codes and times.
Dependencies: RobstrideMotorDiagnostics.h, DiagnosticCsv.h.
Testing: G++ C++17 on PC only; NOT hardware-verified.
Version: 0.1
************************************************/
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include "../../Teensy/communication/RobstrideMotorDiagnostics.h"
#include "DiagnosticCsv.h"

static Robstride::CanFrame detailedFrame(uint8_t motorId,
                                        uint32_t faults, uint32_t warnings) {
    Robstride::CanFrame frame{};
    frame.id = (uint32_t{0x15} << 24) |
               (static_cast<uint32_t>(motorId) << 8) | 1u;
    frame.length = 8;
    frame.extended = true;
    for (int i = 0; i < 4; ++i) {
        frame.data[i] = static_cast<uint8_t>(faults >> (8 * i));
        frame.data[i + 4] = static_cast<uint8_t>(warnings >> (8 * i));
    }
    return frame;
}

int main() {
    DiagnosticBuffer history;
    Robstride::MotorDiagnostics motors(
        history, Robstride::FaultByteOrder::LITTLE_ENDIAN_ORDER);
    assert(motors.registerMotor(3, 0, 1000));
    assert(motors.registerMotor(5, 0, 1000));

    assert(motors.receive(detailedFrame(3, 1u << 2, 0), 100) ==
           Robstride::ProcessResult::ACCEPTED);
    assert(motors.receive(detailedFrame(5, 0, 1), 200) ==
           Robstride::ProcessResult::ACCEPTED);
    assert(motors.receive(detailedFrame(3, 0, 0), 300) ==
           Robstride::ProcessResult::ACCEPTED);
    assert(history.size() == 3);

    const std::string path = "TestBench/src/robstride_diagnostics.csv";
    assert(exportDiagnosticCsv(history, path));
    std::ifstream csv(path);
    assert(csv.is_open());
    const char* expected[] = {
        "timestamp_ms,sensor_id,event_code,severity",
        "100,3,3002,WARNING",
        "200,5,3100,WARNING",
        "300,3,3502,INFO"
    };
    std::string line;
    for (const char* item : expected) {
        assert(static_cast<bool>(std::getline(csv, line)));
        if (!line.empty() && line.back() == '\r') line.pop_back();
        assert(line == item);
    }
    assert(!std::getline(csv, line));
    std::cout << "Robstride -> DiagnosticBuffer -> CSV PASSED\n";
    std::cout << "Generated: " << path << "\n";
    return 0;
}
