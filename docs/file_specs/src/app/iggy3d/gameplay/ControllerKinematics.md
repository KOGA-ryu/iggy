# File Spec

Files: `src/app/iggy3d/gameplay/ControllerKinematics.hpp`, `src/app/iggy3d/gameplay/ControllerKinematics.cpp`

Verified at: `b60dce77`

## Owns

- Product manual first-person horizontal movement math.
- Camera-yaw-relative move direction, walk/sprint speed/profile lookup, step delta, desired velocity, velocity acceleration, and horizontal speed clamp.

## Does Not Own

- Runtime movement simulation, collision resolution, player transform mutation, input polling, tuning table definitions, wall-run evaluation, or movement proof recording.

## Reads

- Move axes, camera yaw, sprint flag, response multiplier, current/target velocity, and `ProductGameplayMovementTuning`.

## Writes / Mutates

- Returns `Vec3`, speed, or profile values only.
- Does not mutate window state, session state, runtime world, collision surfaces, or tuning defaults.

## Calls Out To / Wires Out To

- Uses standard math and `Vec3` operations.
- Reads tuning values supplied by `MovementTuning.hpp`.
- Does not call runtime, save, render, receipt, or session systems.

## Called By / Entry Points

- Move, dash, jump, wall-query, and movement proof paths.
- Grep proof: `rg -n "productManualFirstPersonDirection|productManualFirstPersonMoveDelta|productManualFirstPersonDesiredVelocity|moveProductHorizontalVelocityToward|clampProductHorizontalVelocity" src/app/iggy3d tests/unit`.

## Invariants

- Yaw zero forward maps to negative Z and yaw ninety forward maps to positive X.
- Diagonal input is normalized to avoid faster diagonal movement.
- Zero input direction returns camera forward, but desired velocity returns zero.
- Velocity movement is horizontal-only and clamps by X/Z speed.
- Non-finite velocity distance falls back to target rather than producing invalid intermediate values.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_kinematics_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "productManualFirstPersonDirection|productManualFirstPersonMoveDelta|productManualFirstPersonDesiredVelocity|moveProductHorizontalVelocityToward|clampProductHorizontalVelocity" tests/unit/product_gameplay_controller_kinematics_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/MovementTuning.hpp` unless tuning fields/defaults change.
- `src/runtime/movement/*` unless runtime movement contracts change.
- `src/app/iggy3d/gameplay/ControllerMoveActions.*` unless action integration changes.

## Update When

- Product movement direction, speed/profile lookup, delta/velocity calculation, acceleration, or horizontal clamp semantics change.

## Do Not Update When

- Only movement input bindings, runtime collision, HUD/receipt proof fields, or tuning values change without changing kinematic helper contracts.
