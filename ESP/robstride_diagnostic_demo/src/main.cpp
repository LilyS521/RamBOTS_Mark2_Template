/************************************************
Program: ESP32 standalone Robstride diagnostic demo
Purpose: Run shared diagnostic parser on an ESP32 using
         SYNTHETIC frames and print RAM history over USB.
Inputs: Hardcoded test frames, not real CAN traffic.
Outputs: USB Serial log; never transmits motor CAN.
Dependencies: Arduino core + same Teensy headers used on PC.
Testing: Not verified on physical ESP32; bench demo only.
Version: 0.1
************************************************/
#include <Arduino.h>
#include "../../../Teensy/communication/RobstrideMotorDiagnostics.h"

// Intentionally separate from ESP/app/main.cpp (Lily's BLE application).
static DiagnosticBuffer history;
static Robstride::MotorDiagnostics motors(
    history, Robstride::FaultByteOrder::LITTLE_ENDIAN_ORDER);

static Robstride::CanFrame syntheticFault(uint8_t motor, uint32_t faultMask) {
    Robstride::CanFrame frame{};
    frame.extended = true;
    frame.length = 8;
    frame.id = (uint32_t{0x15} << 24) |
               (static_cast<uint32_t>(motor) << 8) | 1u;
    for (int i = 0; i < 4; ++i)
        frame.data[i] = static_cast<uint8_t>(faultMask >> (8 * i));
    return frame;
}

void setup() {
    Serial.begin(115200);
    delay(1500); // USB serial monitor setup only
    Serial.println("RamBOTs - SYNTHETIC Robstride diagnostic demo");

    motors.registerMotor(3, 0, 1000);
    motors.registerMotor(5, 0, 1000);
    motors.receive(syntheticFault(3, 1u << 2), 100); // Undervoltage
    motors.receive(syntheticFault(3, 1u << 2), 150); // Duplicate suppressed
    motors.receive(syntheticFault(5, 1u << 14), 200); // Stall/overload
    motors.receive(syntheticFault(3, 0), 300); // Fault cleared
    motors.checkTimeouts(1302); // Simulated comm loss for both motors

    Serial.print("Retained events: ");
    Serial.println(static_cast<unsigned>(history.size()));
    DiagnosticRecord event{};
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (history.get(i, event)) {
            Serial.print("t="); Serial.print(event.timestampMs);
            Serial.print(" motor="); Serial.print(event.sensorId);
            Serial.print(" code="); Serial.println(event.code);
        }
    }
    Serial.println("No CAN wiring or motor-control output is used.");
}
void loop() {
    // Deliberately empty: no motor or CAN hardware control.
}
