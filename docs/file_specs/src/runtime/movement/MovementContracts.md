# Movement Contracts

File:

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementCommand.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementParams.hpp`

Verified at: `69514d40`

## Owns

- Shared runtime movement packet types: `MovementRequest`, `MovementResult`, and `KinematicMovementRequest`.
- Movement enums for mode and blocked reasons.
- Default `MovementParams` values used by movement and motor systems.
- Lightweight success/block helpers and tactical/walk mode helper.

## Does Not Own

- Movement request validation or world transform mutation; that belongs to `MovementSystem`.
- Player motor jump/dash/airborne state.
- Physics planner internals.
- App input/controller command construction.

## Reads

- Entity ids, command ids, destinations, movement mode, movement params, and debug result payload types.
- Physics debug packet types carried in `MovementResult`.

## Writes / Mutates

- Header-only contracts; no runtime mutation.
- Callers populate request/result structs.

## Calls Out To / Wires Out To

- `MovementSystem` consumes these packets for movement execution.
- Session and gameplay code use command ids and result fields for proof/debug surfaces.
- Debug/HUD code reads the result fields as observability, not movement truth.

## Called By / Entry Points

- `movementSucceeded(...)`
- `movementBlocked(...)`
- `movementModeForClock(...)`

## Invariants

- `MovementResult::blocked == None` is the success contract.
- Result physics/debug vectors are observational payloads, not persistent physics storage.
- `MovementParams` stays generic movement tuning, not app input policy.

## Tests / Proof Commands

- `rg -n "MovementRequest|MovementResult|MovementParams|MovementBlockedReason" src/runtime tests/unit cmake/iggy3d_tests.cmake`
- `movement_system_tests`, `player_motor_tests`, and `player_physics_move_planner_tests` exercise consumers.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerMotor.*`
- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerPhysicsMovePlanner.*`

## Update When

- Movement request/result fields, blocked reason enum, mode enum, or default movement params change.

## Do Not Update When

- Only execution policy, physics planner math, or app control mapping changes.
