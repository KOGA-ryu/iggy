# File Spec

Files: `src/runtime/ai/NpcSoundPerception.hpp`, `src/runtime/ai/NpcSoundPerception.cpp`

Verified at: `abe51abb`

## Owns

- Stateless NPC sound-hearing math and packet types.
- `SoundEvent`, `SoundPerceptionConfig`, `SoundPerceptionResult`.
- Audibility attenuation, strict hearing threshold, alert-unit conversion, range reject, and highest-heard event reduction.

## Does Not Own

- Sound event production, session sound bus lifetime, wall/occlusion queries, or alert-state mutation.
- Actor memory, investigation steering, movement, or save/hash persistence.
- UI/debug projection of sound facts.

## Reads

- Caller-provided sound event span, listener position, config, and optional blocker flags.
- `Vec3` math for distance.

## Writes / Mutates

- No external state.
- Returns scalar math results or one `SoundPerceptionResult`.

## Calls Out To / Wires Out To

- `SessionTick.cpp` creates footstep `SoundEvent` packets.
- `Session.cpp` filters the transient sound bus, supplies blocker flags, calls `resolveLoudestSound(...)`, and feeds alert stepping.
- `NpcBehaviorProfile` stores per-profile `SoundPerceptionConfig`.

## Called By / Entry Points

- Public entry points: `soundAudibilityDb(...)`, `hearsSound(...)`, `soundToAlertUnits(...)`, `soundAudibleRangeMeters(...)`, `resolveLoudestSound(...)`.
- Grep proof: `rg -n "resolveLoudestSound|soundAudibilityDb|SoundEvent|SoundPerceptionConfig" src tests`.

## Invariants

- Distance is floored at one meter for logarithmic attenuation.
- `hearsSound(...)` is strict greater-than; exactly at threshold is not heard.
- One blocker flag applies one wall-loss, not blocker-count scaling.
- `resolveLoudestSound(...)` range-rejects before audibility math and returns the highest heard event for the tick.
- Investigate position is the event origin.

## Tests / Proof Commands

- `rg -n "npc_sound_perception_tests|session_tick_tests|stealth_garden_tests" cmake tests`.
- `rg -n "SoundEvent|resolveLoudestSound|soundAudibilityDb" src/runtime tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/session/SessionState.hpp` unless the transient sound bus packet changes.
- `src/runtime/session/Session.cpp` unless sound-to-alert wiring changes.
- `src/runtime/ai/NpcAlertSystem.*` unless alert-unit consumption changes.
- `src/runtime/ai/SegmentOcclusion.*` unless blocker verdict semantics change.

## Update When

- Attenuation math, threshold semantics, blocker handling, alert-unit mapping, event/result fields, or highest-wins reduction changes.

## Do Not Update When

- A caller produces more events or changes when hearing is evaluated without changing this kernel.
