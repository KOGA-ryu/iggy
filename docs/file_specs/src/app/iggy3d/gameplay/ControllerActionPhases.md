# File Spec

Files: `src/app/iggy3d/gameplay/ControllerActionPhases.hpp`, `src/app/iggy3d/gameplay/ControllerActionPhases.cpp`

Verified at: `a65a2f58`

## Owns

- Product gameplay input phase ordering for jump timing, dash, retained movement, target actions, reset, and movement proof publishing.
- Wall-run candidate evaluation and publication around active movement state.
- Dash short-circuit behavior after accepted dash input.

## Does Not Own

- Input polling/binding, individual jump/dash/move/target/reset command semantics, collision freshness rebuild, runtime movement kernels, or rendering.

## Reads

- `Session`, `ProductGameplayInputIntent`, `ProductAppWindowState`, source string, collision surfaces, player entity, move axes, jump/dash/interact/attack/reset flags, and wall-run state.

## Writes / Mutates

- Mutates window gameplay proof/state by delegating to jump, dash, move, target, reset, wall-run, and movement proof helpers.
- May mutate session through delegated command submissions.

## Calls Out To / Wires Out To

- `advanceProductJump(...)`, `submitProductJump(...)`, `applyProductJumpReleaseCut(...)`.
- `submitProductDash(...)`.
- `submitProductMove(...)`.
- `submitProductTargetCommand(...)`.
- `submitProductReset(...)`.
- `evaluateProductWallRun(...)`, `publishProductWallRunEvaluation(...)`, and movement proof updates.

## Called By / Entry Points

- `applyProductGameplayActionPhases(...)`.
- `Controller.cpp` live gameplay action application.
- Grep proof: `rg -n "applyProductGameplayActionPhases|submitProductDash|submitProductMove|submitProductTargetCommand|submitProductReset" src/app/iggy3d tests/unit`.

## Invariants

- Jump timing/cooldown advances before dash/move/target/reset phases.
- Dash consumes the frame's remaining gameplay action phases after publishing movement proof.
- Movement proof is published after regular action phases.
- Collision surfaces are passed through but not rebuilt here.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_window_input_frame_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "dash|jump|attack|reset|movement" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/Controller.cpp` unless live controller entrypoint changes.
- `src/app/iggy3d/gameplay/ControllerJumpActions.*` unless jump phase semantics change.
- `src/app/iggy3d/gameplay/ControllerDashActions.*` unless dash phase semantics change.

## Update When

- Gameplay action phase order, dash short-circuit, wall-run evaluation placement, proof publication timing, or delegated action set changes.

## Do Not Update When

- Only individual action internals, input bindings, active room collision building, or rendering changes.
