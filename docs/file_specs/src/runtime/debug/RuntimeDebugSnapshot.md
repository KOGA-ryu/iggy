# Runtime Debug Snapshot

File:

- `/Users/kogaryu/iggy3d/src/runtime/debug/RuntimeDebugSnapshot.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/debug/RuntimeDebugSnapshot.cpp`

Verified at: `fd3abdec`

## Owns

- Runtime debug snapshot request/status/result packet shape.
- Read-only aggregation of player position, speed, spawn distance, motor, movement, slope, ground, traversal, and camera angle facts.
- Runtime debug snapshot reason/status names.
- Actor fallback selection from player roster when request actor is absent.

## Does Not Own

- Simulation state mutation.
- Player motor or movement execution.
- Traversal preview/execution logic.
- Debug projection rendering or app HUD layout.
- Save/hash persistence of debug facts.

## Reads

- `SessionState` world, players, and clock tick.
- Optional previous/spawn positions, delta seconds, yaw, and pitch.
- Optional `PlayerMotorState`, `PlayerMotorResult`, `MovementResult`, traversal intent/result/preview packets.

## Writes / Mutates

- Does not mutate runtime state.
- Returns a `RuntimeDebugSnapshot` with copied/derived facts.

## Calls Out To / Wires Out To

- Reads player roster slot ownership to infer an actor.
- Reads world entity transform/active state.
- Uses movement/player/traversal helper names and success predicates.
- Projection/debug layers append or project runtime debug snapshots downstream.

## Called By / Entry Points

- `buildRuntimeDebugSnapshot(...)`
- `runtimeDebugSnapshotStatusName(...)`

## Invariants

- Disabled requests return disabled snapshot without requiring a session.
- Enabled requests require a session, finite nonnegative delta, valid active actor, and world entity row.
- Actor inference uses the first actor-controlling playable slot with a valid actor.
- Snapshot is observability only; it must not become simulation authority.
- Speed is available only when delta seconds is positive.
- Motor result facts override motor state facts when the result succeeded.
- Traversal intent facts take precedence over direct traversal result facts; preview facts can coexist.

## Tests / Proof Commands

- `rg -n "runtime_debug_snapshot_tests|buildRuntimeDebugSnapshot|RuntimeDebugSnapshotStatus|appendRuntimeDebugSnapshot" cmake/iggy3d_tests.cmake tests/unit src/runtime src/projection`
- `cmake/iggy3d_tests.cmake` registers `runtime_debug_snapshot_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerMotor.*`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementTraversal.*`
- `/Users/kogaryu/iggy3d/src/projection/debug/*`
- `/Users/kogaryu/iggy3d/src/app/iggy3d/debug/*`

## Update When

- Snapshot request/result fields, status semantics, actor fallback, copied motor/movement/traversal facts, or debug reason codes change.

## Do Not Update When

- Only simulation logic, traversal execution, projection formatting, or HUD rendering changes without changing snapshot contract.
