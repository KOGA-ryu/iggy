# File Spec

Files: `src/app/iggy3d/creative/mutation/Metrics.hpp`, `src/app/iggy3d/creative/mutation/Metrics.cpp`

Verified at: `c5537031`

## Owns

- Lightweight creative facade counters.
- `Stats` packet for frames, packets, handled/ignored counts, command attempts/results, object creation, and room creation.
- Reset and increment helpers for command/object/room counters.

## Does Not Own

- Command execution policy.
- UI receipt formatting.
- Persistent save/hash state.
- Runtime physics or gameplay metrics.
- Per-tool timing or performance profiling.

## Reads

- Caller-owned `Stats` references only.

## Writes / Mutates

- Mutates caller-owned `Stats` counters.
- `resetStats(...)` replaces the packet with default values.

## Calls Out To / Wires Out To

- Used by `creative::Facade` to record command attempts and reset state.
- Tests inspect default stability through facade/core surfaces.

## Called By / Entry Points

- `resetStats(...)`.
- `recordCommandAttempt(...)`, `recordCommandSuccess(...)`, `recordCommandFailure(...)`.
- `recordObjectCreated(...)`, `recordRoomCreated(...)`.
- Grep proof: `rg -n "Stats|resetStats|recordCommandAttempt|recordCommandSuccess|recordCommandFailure|recordObjectCreated|recordRoomCreated" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Counters are unsigned monotonic increments except reset.
- This packet is observability only and must not become command truth.
- Reset must restore the same values as default construction.

## Tests / Proof Commands

- `creative_core_tests`.
- `creative_facade_tests`.
- `rg -n "creative_core_tests|creative_facade_tests|recordCommandAttempt|resetStats" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/Facade.*` unless stats ownership changes.
- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless command result accounting changes.
- `src/app/iggy3d/creative/ui/UiFrame.*` unless UI reads new counters.

## Update When

- Creative stats fields, reset semantics, counter increments, or facade stats ownership changes.

## Do Not Update When

- Only command behavior changes without changing stats fields or recording policy.
