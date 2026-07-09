# File Spec

Files: `src/app/iggy3d/gameplay/ControllerJumpDashState.hpp`, `src/app/iggy3d/gameplay/ControllerJumpDashState.cpp`

Verified at: `f6abfbf4`

## Owns

- Product gameplay jump and dash state helper mutations.
- Jump position/height proof, jump timing clear/buffer/cut helpers, dash cooldown advancement, and jump/dash rejection proof fields.

## Does Not Own

- Jump command execution, dash movement execution, traversal lookup, wall-run evaluation, player transform mutation, runtime movement, HUD rendering, or receipt field writing.

## Reads

- `ProductAppWindowState::gameplay.gameplayJump`, `gameplayDash`, and `gameplayMovement.tuning`.
- Tuning fields for jump buffer, jump cut multiplier, dash cooldown step, and input step seconds.

## Writes / Mutates

- Mutates jump ground/start/final/height, coyote/buffer timing, held/cut flags, active velocity on jump release, jump rejection status, dash cooldown, and dash rejection status.
- Does not mutate runtime `Session`, player position, collision surfaces, or active room state.

## Calls Out To / Wires Out To

- No runtime calls; this file writes product window gameplay mirrors directly.
- Uses standard clamp/max helpers for cut multiplier and cooldown decay.

## Called By / Entry Points

- Jump actions, dash actions, action phase handling, reset/fall handling, and wall-run exit handling.
- Grep proof: `rg -n "recordProductJumpPosition|clearProductJumpTiming|rejectProductJump|productJumpBufferLive|bufferProductJump|applyProductJumpReleaseCut|advanceProductDashCooldown|rejectProductDash" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Jump height is clamped to non-negative distance above ground.
- Clearing jump timing resets coyote, buffer, held, and cut flags together.
- Jump release cut only applies while active, upward, and not already cut.
- Dash cooldown never goes below zero.
- Rejection helpers mark the request attempted and accepted false while preserving supplied status/reason strings.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_window_input_frame_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "gameplayJump|gameplayDash|already_airborne|gameplay_dash_cooldown|cutApplied|bufferSecondsRemaining" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_window_input_frame_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerJumpActions.*` unless jump execution flow changes.
- `src/app/iggy3d/gameplay/ControllerDashActions.*` unless dash execution flow changes.
- `src/app/iggy3d/gameplay/MovementTuning.hpp` unless timing/default tuning fields change.

## Update When

- Jump/dash proof fields, timing reset, buffer/coyote/cut behavior, cooldown decay, or rejection status mutation changes.

## Do Not Update When

- Only wall-run query logic, traversal intent selection, runtime movement, renderer, or receipt layout changes without changing jump/dash state helper contracts.
