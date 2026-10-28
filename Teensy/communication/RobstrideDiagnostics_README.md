# Robstride motor diagnostics — RamBOTs Mark II

## Status
PC-tested read-only prototype for **Robstride 03**, designed to support
Robstride 04 where the documented private CAN diagnostic format matches.
No real motor frames or Teensy CAN hardware have been tested.

## Components

| File | Purpose | Inputs | Outputs |
| --- | --- | --- | --- |
| `RobstrideDiagnosticParser.h` | Decode type-2 identifier status and type-21 payload | Extended 29-bit CAN ID, 8-byte payload, selected byte order | Motor ID, host ID, fault/warning masks, names |
| `RobstrideMotorDiagnostics.h` | Track at most 16 configured motors, suppress duplicates, detect missing messages | Parsed incoming frames and monotonic timestamps | Per-motor active fault state, additions to shared `DiagnosticBuffer` |
| `TestBench/src/robstride_diagnostics_test.cpp` | Validate parser and tracker | Synthetic CAN frames | 12 PASS/FAIL tests |
| `TestBench/src/robstride_csv_integration_test.cpp` | Validate existing CSV export | Synthetic transitions | CSV with timestamps, sensor IDs, event codes, severity |
| `ESP/robstride_diagnostic_demo/` | Optional standalone microcontroller demonstration | Synthetic CAN frames, USB power | Serial event list |

## Dependencies and integration
- Uses existing `Teensy/error/DiagnosticBuffer.h` and `FaultEvent.h`.
- Requires the existing `DiagnosticRecord` fields `code`, `severity`,
  `timestampMs`, and `sensorId`; `DiagnosticBuffer::add` accepts those fields.
- The laptop CSV test also uses `TestBench/src/DiagnosticCsv.h` unchanged.
- C++17 compatible and no dynamically allocated collections in the new motor code.
- The diagnostic library neither reads physical CAN nor transmits any commands.

## Receiving frames (future hardware CAN adapter)

```cpp
DiagnosticBuffer history;
Robstride::MotorDiagnostics motors(history,
    Robstride::FaultByteOrder::LITTLE_ENDIAN_ORDER); // PROVISIONAL ONLY
motors.registerMotor(3, 0, 1000); // illustrative timeout, not a final spec

// After a REAL receive-only CAN driver gives you a frame:
// Robstride::CanFrame frame = ...; // id, data, length, extended, remote
// motors.receive(frame, millis());
// motors.checkTimeouts(millis()); // called periodically
```

No motor interface or safety interlock should take action based on the
prototype's event-code mapping. Real fault state remains accessible via
`MotorDiagnostics::snapshot(motorId, out)` even when circular history rolls over.

## Documented CAN format
- Type `0x02`: 29-bit ID: bits 28–24 type; bits 23–22 mode; bits 21–16
  six summary fault bits; bits 15–8 motor ID; bits 7–0 host ID.
- Type `0x15`: 29-bit ID includes motor ID and host ID as above; bytes 0–3
  detailed fault mask and bytes 4–7 warning mask.
- Detailed fault bits documented in manuals: 0, 1, 2, 3, 4, 5, 7, 8, 9, 14, 16.
  Warning bit 0: motor overtemperature warning. Unknown bits remain numeric.
- Byte ordering for the type-21 32-bit fields **has not been verified from
  physical motor captures**. Both endian interpretations are implemented.

## Provisional event codes
| Range | Meaning |
| --- | --- |
| 3000–3031 | Detailed fault bit becomes active |
| 3100–3131 | Detailed warning bit becomes active |
| 3200–3205 | Type-2 summary fault becomes active |
| 3500–3531 | Detailed fault bit clears |
| 3600–3631 | Detailed warning bit clears |
| 3700–3705 | Type-2 summary fault clears |
| 3900 / 3901 | Motor communication timeout / recovery |

Every record's `sensorId` is used as **motor ID**. The name is historical; a
future project-wide record type could rename this field to `sourceId`.
All fault active events use provisional `WARNING` severity and clear events
use `INFO` until the team decides fault severity and safety policy. This module
makes no claim that WARNING means safe to continue operation.

## How to run PC tests
From repository root, with G++ installed:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic TestBench/src/robstride_diagnostics_test.cpp -o TestBench/src/robstride_diagnostics_test.exe
.\TestBench\src\robstride_diagnostics_test.exe

g++ -std=c++17 -Wall -Wextra -pedantic TestBench/src/robstride_csv_integration_test.cpp -o TestBench/src/robstride_csv_integration_test.exe
.\TestBench\src\robstride_csv_integration_test.exe
```

Expected: `All 12 Robstride diagnostic tests passed!` and
`Robstride -> DiagnosticBuffer -> CSV PASSED`.

Generated CSV: `TestBench/src/robstride_diagnostics.csv`.
Add that filename to repository `.gitignore` so generated logs are not committed.

## Important limitations / engineering decisions before hardware
1. The five-event `DiagnosticBuffer` overwrites history. Continuous persistent
   logging requires a separate design; loss of history does not clear active
   state in `MotorDiagnostics`, but this is **not** a safety controller.
2. Summary and detailed flags are tracked as **separate categories**, so the
   same underlying condition can produce two different diagnostic events.
3. Unknown detailed bits are logged numerically, not silently discarded.
4. No event is logged repeatedly unless the associated bit actually changes.
5. Communication timeout starts at registration by default; it requires an
   **expected reporting rate** that must be established by the team. Robstride
   active reports are disabled by default according to the manual; missing
   replies cannot be called a communication failure unless messages are expected.
6. CAN frames are processed in **one task**, not concurrent ISRs. No mutexes,
   thread safety, hard real-time guarantee, allocation audit, or worst-case
   CPU measurement has been performed.
7. A stale recovered indication or sensor drop could be misleading if valid
   frames are sparse or periodic reporting isn't actually enabled.
8. This code does not turn a raw fault into a motor stop, E-stop, or other
   protective action. Safety needs a separate reviewed path.
9. Real CAN acceptance filters should also verify destination host IDs and
   provenance; this prototype registers expected motor IDs only.
10. Unit tests use C++ assertions; compile without `-DNDEBUG`.

## References
- Robstride 03 manual: https://wiki.aifitlab.com/robstride-docs/robstride-03-instruction-manual
- Robstride 04 manual: https://wiki.aifitlab.com/robstride-docs/robstride-04-instruction-manual
- Community reference (RS02-based): https://github.com/masayuki-kono/robstride_driver/blob/main/docs/protocol.md
