# ESP32 read-only diagnostic demonstration (optional)

This standalone demo uses **synthetic** Robstride fault frames to exercise the
same parser and motor state tracker that run in the PC tests. It does not receive
real CAN traffic, transmit CAN, clear errors, change motor state, or interface
with Lily's existing `ESP/app/main.cpp` BLE code.

**Physical board:** SparkFun Thing Plus ESP32-WROOM USB-C (WRL-20168).

## Files
- `src/main.cpp`: USB Serial demonstration using hardcoded frames.
- `platformio.ini.example`: tentative PlatformIO starting point, not a verified
  board configuration for WRL-20168. Confirm the PlatformIO board profile with
  the board's exact SKU and official schematics before copying to `platformio.ini`.

## Dependencies
- PlatformIO with Espressif32 + Arduino framework (or suitable Arduino build).
- Existing `Teensy/error/DiagnosticBuffer.h` and `FaultEvent.h`, plus the two new
  Robstride communication headers in the repository. Keep the repository layout.

## Procedure
1. Complete PC tests first.
2. Open **this demo folder** as a separate PlatformIO project, rather than
   changing Lily's application.
3. Select and verify the correct ESP32-WROOM USB-C board profile. The example
   configuration is a candidate, not hardware-tested.
4. Connect the ESP32 via USB-C only (no CAN wiring and no motors).
5. Build/upload and open Serial Monitor at 115200 baud.
6. Expect text showing simulated motor IDs, event codes, and timestamps.

## Next hardware stage
For real CAN reception, obtain a suitable **3.3 V-compatible CAN transceiver**,
confirm pin mapping with the board schematic, bus termination, bus power and
common reference/ground, and use a receive-only CAN/TWAI configuration for
initial sniffing. An ESP32 logic pin must never be wired directly to CAN_H/L.
The actual motor CAN bitrate and protocol mode must be checked on the bench.

**Hardware status:** Not compiled/uploaded/tested on a physical ESP32 yet.
