# Player Motor

File:

- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerMotor.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerMotor.cpp`

Verified at: `69514d40`

## Owns

- Runtime player motor state, params, input, context, result, phase, and status packets.
- Grounded, airborne, dash, jump, and wire-walk motor updates.
- Ground sampling, slope policy sampling, snap/landing facts, and horizontal collision response for motor movement.
- Optional use of `PlayerPhysicsMovePlanner` for horizontal collision resolution.

## Does Not Own

- Command admission or app input mapping.
- Generic command movement execution in `MovementSystem`.
- Physics planner internals.
- Traversal slot discovery or traversal entry execution.
- Save/load persistence of transient motor state.

## Reads

- Mutable `WorldState` and immutable `SpatialSurfaceSet` from `PlayerMotorContext`.
- `PlayerMotorState`, `PlayerMotorInput`, and `PlayerMotorParams`.
- Actor transform/active flag, ground surfaces, actor blockers, slope samples, and optional physics planner result.

## Writes / Mutates

- Mutates `PlayerMotorState` phase, grounded flag, jump availability, dash timers, velocities, and wire-walk coordinate.
- Mutates actor transform through `WorldState::updateTransform` when movement or snap applies.
- Returns `PlayerMotorResult` with motor state, rejection flags, movement facts, ground facts, slope policy facts, and hit surface id.

## Calls Out To / Wires Out To

- Calls collision `sampleSurfaceHeight` and actor `querySegment` helpers.
- Calls `sampleSlope` for ground policy facts.
- Calls `planPlayerPhysicsMove` when configured to use the physics move planner.

## Called By / Entry Points

- `playerMotorSucceeded(...)`
- `playerMotorStatusName(...)`
- `playerMotorPhaseName(...)`
- `updatePlayerMotor(...)`
- Runtime debug snapshot reads motor state/result packets.

## Invariants

- Missing world/surfaces, invalid params/input, invalid actor, inactive actor, or missing ground for grounded phase return status failures before mutation.
- Ground snap mutates world only through `WorldState::updateTransform`.
- Jump transitions grounded or wire-walk phase to airborne.
- Dash requires horizontal intent and clear cooldown.
- Airborne motion applies acceleration, drag, gravity, terminal velocity, landing snap, and horizontal collision.
- Wire-walk clamps coordinate along the configured rail and can launch into airborne on jump.

## Tests / Proof Commands

- `rg -n "player_motor_tests|updatePlayerMotor|PlayerMotorStatus|PlayerMotorPhase" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `player_motor_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerPhysicsMovePlanner.*`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementTraversal.*`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/debug/RuntimeDebugSnapshot.*`

## Update When

- Motor params, phase/status semantics, state/result packets, world mutation rules, collision handling, jump/dash/wire-walk behavior, or physics-planner handoff changes.

## Do Not Update When

- Only command admission, traversal slot discovery, app input mapping, or render/debug presentation changes.
