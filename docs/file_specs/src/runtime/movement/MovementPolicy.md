# Movement Policy

File:

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementPolicy.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementPolicy.cpp`

Verified at: `69514d40`

## Owns

- Slope band descriptors and default slope bands.
- Slope normal sampling into walkability, angle, up-dot, movement multipliers, and careful-footing facts.
- Mapping slope angle to policy band.

## Does Not Own

- Surface height/normal queries.
- World transform mutation.
- Player motor jump/dash logic.
- Runtime config persistence or app tuning UI.

## Reads

- Surface normal vector.
- `MovementParams::maxWalkableSlopeDegrees`.
- Optional caller-supplied slope band span.

## Writes / Mutates

- Does not mutate runtime state.
- Returns `SlopeSample` packets.

## Calls Out To / Wires Out To

- `MovementSystem` and `PlayerMotor` use slope samples for result policy facts.
- Tests use `defaultSlopeBands`, `resolveSlopeBand`, and `sampleSlope` directly.

## Called By / Entry Points

- `defaultSlopeBands(...)`
- `resolveSlopeBand(...)`
- `sampleSlope(...)`

## Invariants

- Invalid normals or invalid max slope produce an invalid sample.
- Slope angle is derived from normalized normal dot world up.
- Walkable requires slope angle within max walkable slope, positive speed multiplier, and upward-facing normal.
- Invalid angle or empty bands resolve to the default blocked band.

## Tests / Proof Commands

- `rg -n "movement_policy_tests|defaultSlopeBands|resolveSlopeBand|sampleSlope" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `movement_policy_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementParams.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerMotor.*`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementSystem.*`

## Update When

- Default slope bands, walkability rules, multiplier fields, careful-footing semantics, or slope sample fields change.

## Do Not Update When

- Only collision surface generation, motor state, or app HUD wording changes.
