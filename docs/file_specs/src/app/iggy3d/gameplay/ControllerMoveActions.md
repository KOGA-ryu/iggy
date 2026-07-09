# File Spec

Files: `src/app/iggy3d/gameplay/ControllerMoveActions.hpp`, `src/app/iggy3d/gameplay/ControllerMoveActions.cpp`

Verified at: `74fcbc90`

## Owns

- Product manual movement submission for grounded retained velocity and airborne X/Z control.
- Horizontal ground velocity retention, acceleration/deceleration, sprint speed selection, and zero-intent decay.
- Airborne movement fallback that mutates player X/Z directly while jump/fall owns vertical motion.
- `productHorizontalVelocityActive(...)` proof helper.

## Does Not Own

- Input phase ordering, collision surface freshness, command admission internals, movement kernel internals, jump/fall vertical integration, or rendering.
- Dash, target, reset, or wall-run candidate scheduling.

## Reads

- `Session`, `ProductAppWindowState`, move axes, sprint flag, camera yaw, movement tuning, jump/wall-run state, player entity, and optional collision surfaces.

## Writes / Mutates

- Mutates window gameplay movement proof fields, retained ground velocity, input source, and movement profile.
- Airborne movement mutates player position directly through player access helpers.
- Grounded movement submits a local-player move command through gameplay command execution.

## Calls Out To / Wires Out To

- `productManualFirstPersonDesiredVelocity(...)`, `moveProductHorizontalVelocityToward(...)`, and velocity clamps.
- `setProductPlayerPosition(...)` for airborne movement.
- `submitProductGameplayCommand(...)` for grounded move commands.
- Wall-run tangent helpers and movement proof helpers.

## Called By / Entry Points

- `productHorizontalVelocityActive(...)`.
- `submitProductMove(...)`.
- `ControllerActionPhases.*`.
- Grep proof: `rg -n "submitProductMove|productHorizontalVelocityActive|airborne_manual_move" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Missing player clears retained ground velocity and reports missing-player command status.
- Airborne movement does not submit a runtime command and must not overwrite jump-owned vertical motion.
- Grounded zero retained delta returns without submitting a command.
- Grounded movement commands must carry local-player source and target point.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_window_input_frame_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "groundVelocity|airborne_manual_move|manual move|sprint move" tests/unit/product_gameplay_controller_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerActionPhases.*` unless movement scheduling changes.
- `src/app/iggy3d/gameplay/ControllerCommandExecution.*` unless command handoff changes.
- `src/app/iggy3d/gameplay/MovementTuning.*` unless tuning fields or movement profile names change.

## Update When

- Manual movement math, retained velocity policy, airborne movement policy, status/reason fields, or command handoff changes.

## Do Not Update When

- Only input key binding, active room collision bake, target actions, or renderer receipt formatting changes.
