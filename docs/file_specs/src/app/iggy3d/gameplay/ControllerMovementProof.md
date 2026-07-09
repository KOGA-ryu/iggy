# File Spec

Files: `src/app/iggy3d/gameplay/ControllerMovementProof.hpp`, `src/app/iggy3d/gameplay/ControllerMovementProof.cpp`

Verified at: `08813cf2`

## Owns

- Product-side movement debug proof copied from runtime session transient movement results.
- Product movement state derivation for grounded, airborne, wall-running, blocked/sliding, and idle/moving states.
- Manual movement profile proof and local airborne/ledge-fall movement debug packets.

## Does Not Own

- Runtime movement planning, collision resolution, player command admission, input dispatch, HUD rendering, receipt writing, or movement tuning descriptor definitions.

## Reads

- `Session::state().transient.lastMovementResult` and movement result fields.
- `ProductAppWindowState::gameplay` movement, jump, wall-run, and tuning mirrors.
- Runtime movement helpers for travel facts, reason names, profile names, and max speed.

## Writes / Mutates

- Clears or fills `window.gameplay.gameplayMovement` debug fields.
- Updates movement state, grounded flag, horizontal speed, movement profile, max speed, and travel facts.
- Does not mutate runtime `Session`.

## Calls Out To / Wires Out To

- `computeMovementTravelFacts(...)`, `movementTravelDirectionName(...)`, and `movementBlockedReasonName(...)`.
- `productManualFirstPersonMovementProfile(...)` and `productManualFirstPersonMaxSpeedMetersPerSecond(...)`.
- `nearlyEqual(...)` for movement debug position-change proof.

## Called By / Entry Points

- `recordProductMovementDebug(...)`, `updateProductMovementStateProof(...)`, and helper functions in this file.
- Called from gameplay command execution, movement actions, and action phase cleanup.
- Grep proof: `rg -n "recordProductMovementDebug|updateProductMovementStateProof|recordProductMovementProfile|productHorizontalMovementSpeedMetersPerSecond" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Missing runtime movement debug clears product movement proof to `not_requested`.
- Horizontal speed is the max of retained velocity and debug travel distance over input step.
- Blocked, clamped, or sliding movement wins over active movement states.
- Active jump state splits into wall-running, airborne manual control, jumping, rising, or falling.
- Local airborne and ledge-fall debug packets must use movement travel facts, not runtime collision sweeps.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_movement_debug_hud_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "ProductGameplayMovementState|gameplayMovement.state|recordProductMovementDebug|horizontalSpeedMetersPerSecond" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_movement_debug_hud_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/movement/*` unless runtime movement result semantics change.
- `src/app/iggy3d/debug/MovementDebugHud.*` unless HUD field formatting changes.
- `src/app/iggy3d/gameplay/MovementTuning.hpp` unless tuning/state descriptors change.

## Update When

- Product movement proof fields, state derivation, profile proof, horizontal speed proof, or runtime movement-result copying changes.

## Do Not Update When

- Only movement input bindings, renderer layout, receipt field ordering, or runtime planner internals change without changing product proof semantics.
